#include "Algorithm.h"

WigginsAlgorithm::WigginsAlgorithm(
	const AlgorithmParameter& parameter,
	const std::vector<float>& stocksdata,
	const std::map<std::string, int>& deviceNameToWorkload
):
	m_parameter{ parameter },
	m_returns{ stocksdata },
	m_response{
		parameter.m_volatility_theta.generateBuffer(parameter.m_DiscretCountOfContinuiosSpace),
		parameter.m_volatility_mu.generateBuffer(parameter.m_DiscretCountOfContinuiosSpace),
		parameter.m_volatility_sigma.generateBuffer(parameter.m_DiscretCountOfContinuiosSpace)
	}
{
	this->iteration_count = 0;
	int workloadTotal = 0;
	std::vector<cl::sycl::device> devices = cl::sycl::device::get_devices();
	std::set<std::string> devices_that_occured;
	
	for (int deviceIndex = 0; deviceIndex < devices.size(); deviceIndex++) {
		std::string deviceName = devices[deviceIndex].get_info<cl::sycl::info::device::name>();
		if (devices_that_occured.find(deviceName) != devices_that_occured.end()) {
			continue;
		}
		else {
			devices_that_occured.insert(deviceName);
		}
		const int workload = deviceNameToWorkload.at(deviceName);
		workloadTotal += workload;
	}
	if (workloadTotal == 0) {
		throw std::runtime_error("Zero workload selected on all devices");
	}
	devices_that_occured.clear();
	for (int deviceIndex = 0; deviceIndex < devices.size(); deviceIndex++) {
		std::string deviceName = devices[deviceIndex].get_info<cl::sycl::info::device::name>();
		if (devices_that_occured.find(deviceName) != devices_that_occured.end()) {
			continue;
		}
		else {
			devices_that_occured.insert(deviceName);
		}
		const int workload = deviceNameToWorkload.at(deviceName);
		if (workload <= 0)
			continue;
		
		cl::sycl::queue q(devices[deviceIndex]);
		int continuousDiscreteQuanta = this->m_parameter.m_DiscretCountOfContinuiosSpace;
		AlgorithmDeviceData algoDeviceData(
			m_response,
			(float)workload / (float)workloadTotal
		);
		m_QueuesAndData.push_back(std::make_pair(q, algoDeviceData));
	}

	this->start = {
		this->m_parameter.m_volatility_theta.testval,
		this->m_parameter.m_volatility_mu.testval,
		this->m_parameter.m_volatility_sigma.testval
	};

}	






float potential_energy_U(
	const ProbabilityDomain domain,
	int timeSeriesLength,
	oneapi::dpl::minstd_rand &engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	float U = 0;
	float volatility = domain.mu;
	oneapi::dpl::normal_distribution<float> Normal_of_zero_n_sigma(0.0, domain.sigma);
	for (int i = 0; i < timeSeriesLength; i++) {
		float v =
			volatility +
			domain.theta * (domain.mu - volatility) +
			Normal_of_zero_n_sigma(engine);
		volatility = v;
		U +=
			v +
			LOG_2_PI_DIVIDED_BY_2 +
			0.5 * pow(returns[i] / exp(v), 2.0);
	}
	return U;
}

static float evaluate_del_v_by_del_sigma(float sigma, float r) {
	float result = MINUS_ONE_UPON_SQRT_2_PI;
	float sigma_square = sigma * sigma;
	float r_square = r * r;
	result *= sigma_square - r_square;
	result *= exp(-0.5 * (r_square / sigma_square));
	result /= sigma_square * sigma_square;
	return result;
}

float potential_energy_U(
	const ProbabilityDomain domain,
	ProbabilityDomain &differential,
	const AlgorithmParameter& parameters,
	int timeSeriesLength,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	float U = 0;
	differential.theta = 0;
	differential.mu = 0;
	differential.sigma = 0;
	float volatility = domain.mu;
	oneapi::dpl::normal_distribution<float> Normal_of_zero_n_sigma(0.0, domain.sigma);
	for (int i = 0; i < timeSeriesLength; i++) {
		float normal_zero_sigma_value = Normal_of_zero_n_sigma(engine);
		float del_v_del_theta = (domain.mu - volatility);
		float del_v_del_mu = domain.theta;
		float del_v_del_sigma = evaluate_del_v_by_del_sigma(domain.sigma, normal_zero_sigma_value);
		float v =
			volatility +
			domain.theta * del_v_del_theta +
			normal_zero_sigma_value;
		volatility = v;
		float res_i_upon_e_v_squred = returns[i] / exp(v); res_i_upon_e_v_squred *= res_i_upon_e_v_squred;
		differential.theta += del_v_del_theta * (1 - res_i_upon_e_v_squred);
		differential.mu += del_v_del_mu * (1 - res_i_upon_e_v_squred);
		differential.sigma += del_v_del_sigma * (1 - res_i_upon_e_v_squred);
		U +=
			v +
			LOG_2_PI_DIVIDED_BY_2 +
			0.5 * res_i_upon_e_v_squred;
	}
	return U;
}

ProbabilityDomain potential_energy_gradient(
	const ProbabilityDomain domain,
	int tsLength,
	const AlgorithmParameter& parameters,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	ProbabilityDomain gradient;
	potential_energy_U(
		domain, gradient, parameters,  tsLength,  engine, returns
	);
	return gradient;
}

bool WigginsAlgorithm::is_completed() {
	const int number_of_iteration = 
		this->m_parameter.m_MCMCIteration / this->m_parameter.m_GraphUpdateIteration
		+ (this->m_parameter.m_MCMCIteration % this->m_parameter.m_GraphUpdateIteration > 0);
	return this->iteration_count >= number_of_iteration;
}

float WigginsAlgorithm::percent_of_completion() {
	const float number_of_iteration =
		this->m_parameter.m_MCMCIteration / this->m_parameter.m_GraphUpdateIteration
		+ (this->m_parameter.m_MCMCIteration % this->m_parameter.m_GraphUpdateIteration > 0);
	return this->iteration_count * 100 / number_of_iteration;
}

AlgorithmResponse WigginsAlgorithm::get_response() {
	return this->m_response;
}

