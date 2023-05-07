#include "Algorithm.h"

std::pair<std::vector<float>, float> UniformParameters::generateBuffer(
	uint32_t DiscretCountOfContinuiosSpace
) {
	const float uniform_probability = 1.0f / (this->upper - this->lower);
	std::vector<float> data((size_t)DiscretCountOfContinuiosSpace, uniform_probability);
	float sum = 1.0f;
	return std::make_pair(data, sum);
}

float evaluate_normal(float x, float mean, float sd) {
	x = x - mean;
	x /= sd;
	x *= x;
	x *= -0.5f;
	x = exp(x);
	x /= sd;
	x /= sqrtf(2 * M_PI);
	return x;
}

float NormalParameter::evaluate(float x) {
	return evaluate_normal(x, this->mean, this->sd);
}
std::pair<std::vector<float>, float> NormalParameter::generateBuffer(
	uint32_t DiscretCountOfContinuiosSpace
) {
	std::vector<float> data(DiscretCountOfContinuiosSpace);
	float sum = 0;
	const float start = this->mean - this->buffer_range_sigma_multiplier * this->sd;
	const float end = this->mean + this->buffer_range_sigma_multiplier * this->sd;
	const float delta_x = (end - start) / DiscretCountOfContinuiosSpace;
	for (int SpaceQuanta = 0; SpaceQuanta < DiscretCountOfContinuiosSpace; SpaceQuanta++) {
		float x = start + delta_x * SpaceQuanta;
		float y = this->evaluate(x);
		sum += y;
		data[SpaceQuanta] = y;
	}
	return std::make_pair(data, sum);
}

WigginsAlgorithm::WigginsAlgorithm(
	const AlgorithmParameter& parameter,
	const std::vector<float>& stocksdata,
	const std::map<std::string, int>& deviceNameToWorkload
):
	m_parameter{ parameter },
	m_returns{ stocksdata }
{
	this->iteration_count = 0;
	int workloadTotal = 0;
	std::vector<cl::sycl::device> devices = cl::sycl::device::get_devices();
	for (int deviceIndex = 0; deviceIndex < devices.size(); deviceIndex++) {
		std::string deviceName = devices[deviceIndex].get_info<cl::sycl::info::device::name>();
		const int workload = deviceNameToWorkload.at(deviceName);
		if (workload <= 0)
			continue;
		
		workloadTotal += workload;
		cl::sycl::queue q(devices[deviceIndex]);
		AlgorithmDeviceData algoDeviceData;
		algoDeviceData.m_workload_fraction = workload;
		m_QueuesAndData.insert(std::make_pair(q, algoDeviceData));
	}
	if (m_QueuesAndData.empty()) {
		throw std::runtime_error("Zero workload selected on all devices");
	}
	for (auto QueuesAndDataMemmber : m_QueuesAndData) {
		QueuesAndDataMemmber.second.m_workload_fraction /= workloadTotal;
	}
}	

static int sycl_strogest_cpu_selector(const sycl::device& d) {
	return d.is_cpu();
}


float WigginsAlgorithm::potential_energy_U(
	const ProbabilityDomain domain,
	int timeSeriesLength,
	oneapi::dpl::uniform_real_distribution<float> &dW,
	oneapi::dpl::minstd_rand &engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	float U = 0;
	float x0 = returns[0];
	float y_time_minus_one;
	float y_time = x0;
	for (int time = 1; time < timeSeriesLength; time++) {
		float x_time = returns[time];
		float dw = dW(engine);
		y_time_minus_one = y_time;
		y_time =
			y_time_minus_one +
			domain.theta * (domain.mu - y_time_minus_one) +
			domain.sigma * dw;
		float p_time = evaluate_normal(x_time, 0, exp(y_time));
		U += p_time;
	}
	return -1 * U;
}

float WigginsAlgorithm::potential_energy_U(
	const ProbabilityDomain domain,
	ProbabilityDomain &differential,
	int timeSeriesLength,
	oneapi::dpl::uniform_real_distribution<float>& dW,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	float U = 0;
	differential.theta = 0;
	differential.mu = 0;
	differential.sigma = 0;
	float x0 = returns[0];
	float y_time_minus_one;
	float y_time = x0;
	for (int time = 1; time < timeSeriesLength; time++) {
		float x_time = returns[time];
		float dw = dW(engine);
		y_time_minus_one = y_time;
		y_time =
			y_time_minus_one +
			domain.theta * (domain.mu - y_time_minus_one) +
			domain.sigma * dw;
		float p_time = evaluate_normal(x_time, 0, exp(y_time));
		float del_p_time_del_y_time =
			(p_time * x_time * x_time) /
			exp(2 * y_time);
		differential.theta += del_p_time_del_y_time  * (domain.mu - y_time_minus_one);
		differential.mu += del_p_time_del_y_time * domain.theta;
		differential.sigma += del_p_time_del_y_time * dw;
		U += p_time;
	}
	differential.theta *= -1;
	differential.mu *= -1;
	differential.sigma *= -1;
	return -1 * U;
}

ProbabilityDomain WigginsAlgorithm::potential_energy_gradient(
	const ProbabilityDomain domain,
	int tsLength,
	oneapi::dpl::uniform_real_distribution<float>& dW,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	ProbabilityDomain gradient;
	this->potential_energy_U(
		domain, gradient, tsLength, dW, engine, returns
	);
	return gradient;
}


bool WigginsAlgorithm::hmc_sample(
	float epsilon,
	int leapfrog,
	const ProbabilityDomain current_q,
	ProbabilityDomain &new_domain,
	oneapi::dpl::normal_distribution<float>& theta_normal,
	oneapi::dpl::normal_distribution<float>& mu_normal,
	oneapi::dpl::normal_distribution<float> & sigma_normal,
	oneapi::dpl::uniform_real_distribution<float> &zero_one_uniform,
	int tsLength,
	oneapi::dpl::uniform_real_distribution<float>& dW,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	ProbabilityDomain q = current_q;
	ProbabilityDomain p{ theta_normal(engine) , mu_normal(engine), sigma_normal(engine) };
	ProbabilityDomain current_p = p;
	p = p - (epsilon * this->potential_energy_gradient(q, tsLength, dW, engine, returns) * 0.5);
	for (int i = 0; i < leapfrog - 1; i++) {
		q = q + epsilon * p;
		p = p - (epsilon * this->potential_energy_gradient(q, tsLength, dW, engine, returns));
	}
	p = p - (epsilon * this->potential_energy_gradient(q, tsLength, dW, engine, returns) * 2.0f);
	float current_U = potential_energy_U(current_q, tsLength, dW, engine, returns);
	float current_K = (current_p * current_p * 0.5).sum();
	float proposed_U = potential_energy_U(q, tsLength, dW, engine, returns);
	float proposed_K = (p * p * 0.5).sum();
	float energy_change = current_U - proposed_U + current_K - proposed_K;
	if (zero_one_uniform(engine) < exp(energy_change)) {
		new_domain = q;
		return true;
	}
	else {
		new_domain = current_q;
		return false;
	}

}

void WigginsAlgorithm::BurnIn() {
	using namespace cl::sycl;
	queue q(sycl_strogest_cpu_selector);
	unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	buffer<float, 1> result(range<1>{3});
	q.submit([&, this](handler &h) {
		const AlgorithmParameter parameter = this->m_parameter;
		accessor returnsAccessor(m_returns, h, read_only);
		int tsLength = m_returns.size();
		h.single_task([=]() {
			oneapi::dpl::minstd_rand engine(seed, 0);
			oneapi::dpl::normal_distribution<float> theta_normal(0.0f, parameter.m_volatility_theta.guassian_step_sd);
			oneapi::dpl::normal_distribution<float> mu_normal(0.0f, parameter.m_volatility_mu.guassian_step_sd);
			oneapi::dpl::normal_distribution<float> sigma_normal(0.0f, parameter.m_volatility_sigma.guassian_step_sd);
			oneapi::dpl::uniform_real_distribution<float> zero_one_uniform(0.0f, 1.0f);
			oneapi::dpl::uniform_real_distribution<float> dW(parameter.m_volatility.dw_lower, parameter.m_volatility.dw_upper);
			ProbabilityDomain startingProbabilityDomain {
				parameter.m_volatility_theta.testval,
				parameter.m_volatility_mu.testval,
				parameter.m_volatility_sigma.testval
			}, nextDomain;
			hmc_sample(
				parameter.m_epsilon, parameter.m_leapfrog,
				startingProbabilityDomain,
				nextDomain,
				theta_normal,
				mu_normal,
				sigma_normal,
				zero_one_uniform,
				tsLength,
				dW,
				engine,
				returnsAccessor
			);
		});
	});
}
