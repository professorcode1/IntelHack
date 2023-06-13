#include "HMC.h"

HMC::HMC(
	const AlgorithmParameter& parameter,
	const std::vector<float>& stocksdata,
	const std::map<std::string, int>& deviceNameToWorkload
):WigginsAlgorithm{ parameter ,stocksdata , deviceNameToWorkload }{
	this->BurnIn();
}

static int sycl_strogest_cpu_selector(const sycl::device& d) {
	return d.is_cpu();
}

bool hmc_sample(
	const ProbabilityDomain current_q,
	ProbabilityDomain& new_domain,
	const AlgorithmParameter& parameters,
	oneapi::dpl::normal_distribution<float>& theta_normal,
	oneapi::dpl::normal_distribution<float>& mu_normal,
	oneapi::dpl::normal_distribution<float>& sigma_normal,
	oneapi::dpl::uniform_real_distribution<float>& zero_one_uniform,
	int tsLength,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	float epsilon = parameters.m_epsilon;
	int leapfrog = parameters.m_leapfrog;
	ProbabilityDomain q = current_q;
	ProbabilityDomain p{ theta_normal(engine) , mu_normal(engine), sigma_normal(engine) };
	ProbabilityDomain current_p = p;
	float mu_lower = parameters.m_volatility_mu.mean - parameters.m_volatility_mu.buffer_range_sigma_multiplier * parameters.m_volatility_mu.sd;
	float mu_higher = parameters.m_volatility_mu.mean + parameters.m_volatility_mu.buffer_range_sigma_multiplier * parameters.m_volatility_mu.sd;

	p = p + (-1.0 * epsilon * potential_energy_gradient(q, tsLength, parameters, engine, returns) * 0.5);
	for (int i = 0; i < leapfrog - 1; i++) {
		q = q + epsilon * p;
		//clamp(parameters.m_volatility_theta.lower, q.theta, parameters.m_volatility_theta.upper);
		//clamp(mu_lower, q.mu, mu_higher);
		//clamp(parameters.m_volatility_sigma.lower, q.sigma, parameters.m_volatility_sigma.upper);

		p = p + (-1.0 * epsilon * potential_energy_gradient(q, tsLength, parameters, engine, returns));
	}
	p = p + (-1.0 * epsilon * potential_energy_gradient(q, tsLength, parameters, engine, returns) * 2.0f);
	float current_U = potential_energy_U(current_q, tsLength, engine, returns);
	ProbabilityDomain current_K_Domain = current_p * current_p * 0.5;
	float current_K = current_K_Domain.mu + current_K_Domain.sigma + current_K_Domain.theta;
	ProbabilityDomain propsed_K_domain = p * p * 0.5;
	float proposed_U = potential_energy_U(q, tsLength, engine, returns);
	float proposed_K = propsed_K_domain.mu + propsed_K_domain.sigma + propsed_K_domain.theta;
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

void HMC::BurnIn() {
	using namespace cl::sycl;
	queue q(sycl_strogest_cpu_selector);
	unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	buffer<float, 1> result(range<1>{3});
	q.submit([&, this](handler& h) {
		const AlgorithmParameter parameter = this->m_parameter;
		accessor returnsAccessor(m_returns, h, read_only);
		accessor resultAccessor(result, h, write_only);
		int tsLength = m_returns.size();
		ProbabilityDomain start = this->start;
		h.single_task([=]() {
			oneapi::dpl::minstd_rand engine(seed, 0);
			oneapi::dpl::normal_distribution<float> theta_normal(0.0f, parameter.m_volatility_theta.guassian_step_sd);
			oneapi::dpl::normal_distribution<float> mu_normal(0.0f, parameter.m_volatility_mu.guassian_step_sd);
			oneapi::dpl::normal_distribution<float> sigma_normal(0.0f, parameter.m_volatility_sigma.guassian_step_sd);
			oneapi::dpl::uniform_real_distribution<float> zero_one_uniform(0.0f, 1.0f);
			ProbabilityDomain oldDomain{ start }, nextDomain;
			for (int i = 0; i < parameter.m_BurnIn; i++) {
				hmc_sample(
					oldDomain,
					nextDomain,
					parameter,
					theta_normal,
					mu_normal,
					sigma_normal,
					zero_one_uniform,
					tsLength,
					engine,
					returnsAccessor
				);
				oldDomain = nextDomain;
			}
			resultAccessor[0] = nextDomain.theta;
			resultAccessor[1] = nextDomain.mu;
			resultAccessor[2] = nextDomain.sigma;
			});
		}).wait();
		host_accessor resultHostAccessor(result, read_write);
		this->start.theta = resultHostAccessor[0];
		this->start.mu = resultHostAccessor[1];
		this->start.sigma = resultHostAccessor[2];
}

uint32_t DistributionIndex(float lower, float upper, uint32_t divisions, float value) {
	float delx = (upper - lower) / divisions;
	uint32_t res = floor((value - lower) / delx);
	if (res == divisions) {
		return res - 1;
	}
	return res;
}


cl::sycl::event exectue_wiggins_algorithm_on_device(
	cl::sycl::queue& q, const AlgorithmDeviceData& device_data, int workItems,
	const AlgorithmParameter& parameter, const ProbabilityDomain& start,
	cl::sycl::buffer<float, 1> returns, cl::sycl::buffer<float, 1>& theta,
	cl::sycl::buffer<float, 1>& mu, cl::sycl::buffer<float, 1>& sigma
) {
	using namespace cl::sycl;

	int tsLength = returns.size();
	unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	//run the MNC Algorithm, write result to theta mu sigma buffer 
	return q.submit([&](handler& h) {
		accessor thetaAccessor(theta, h, write_only);
		accessor muAccessor(mu, h, write_only);
		accessor sigmaAccessor(sigma, h, write_only);
		accessor returnsAccessor(returns, h, read_only);
		h.parallel_for(range<1>(workItems), [=](id<1> thread_id) {
			oneapi::dpl::minstd_rand engine(seed, thread_id);
			oneapi::dpl::normal_distribution<float> theta_normal(0.0f, parameter.m_volatility_theta.guassian_step_sd);
			oneapi::dpl::normal_distribution<float> mu_normal(0.0f, parameter.m_volatility_mu.guassian_step_sd);
			oneapi::dpl::normal_distribution<float> sigma_normal(0.0f, parameter.m_volatility_sigma.guassian_step_sd);
			oneapi::dpl::uniform_real_distribution<float> zero_one_uniform(0.0f, 1.0f);
			ProbabilityDomain oldDomain{ start }, nextDomain;
			hmc_sample(
				oldDomain,
				nextDomain,
				parameter,
				theta_normal,
				mu_normal,
				sigma_normal,
				zero_one_uniform,
				tsLength,
				engine,
				returnsAccessor
			);
			thetaAccessor[thread_id] = nextDomain.theta;
			muAccessor[thread_id] = nextDomain.mu;
			sigmaAccessor[thread_id] = nextDomain.sigma;
			});
		});

}

void HMC::iterate() {
	using namespace cl::sycl;
	std::vector<cl::sycl::event> events;
	std::vector<cl::sycl::buffer<float, 1> > thetaHists;
	std::vector<cl::sycl::buffer<float, 1> > muHists;
	std::vector<cl::sycl::buffer<float, 1> > sigmaHists;
	float mu_lower = this->m_parameter.m_volatility_mu.mean - this->m_parameter.m_volatility_mu.buffer_range_sigma_multiplier * this->m_parameter.m_volatility_mu.sd;
	float mu_higher = this->m_parameter.m_volatility_mu.mean + this->m_parameter.m_volatility_mu.buffer_range_sigma_multiplier * this->m_parameter.m_volatility_mu.sd;
	uint32_t DiscreteQuanta = this->m_parameter.m_DiscretCountOfContinuiosSpace;
	for (int deviceIndex = 0; deviceIndex < this->m_QueuesAndData.size(); deviceIndex++) {
		const int workItems = ceil(this->m_parameter.m_GraphUpdateIteration * this->m_QueuesAndData[deviceIndex].second.m_workload_fraction);

		thetaHists.emplace_back(workItems);
		muHists.emplace_back(workItems);
		sigmaHists.emplace_back(workItems);
		events.push_back(
			exectue_wiggins_algorithm_on_device(
				this->m_QueuesAndData[deviceIndex].first,
				this->m_QueuesAndData[deviceIndex].second, workItems,
				this->m_parameter, this->start,
				this->m_returns,
				thetaHists.back(), muHists.back(), sigmaHists.back()
			)
		);
	}
	for (int deviceIndex = 0; deviceIndex < this->m_QueuesAndData.size(); deviceIndex++) {
		const int workItems = ceil(this->m_parameter.m_GraphUpdateIteration * this->m_QueuesAndData[deviceIndex].second.m_workload_fraction);
		host_accessor thetaList(thetaHists[deviceIndex], read_only);
		for (int index = 0; index < workItems; index++) {
			float theta = thetaList[index];
			uint32_t thetaHistIndex = DistributionIndex(
				this->m_parameter.m_volatility_theta.lower,
				this->m_parameter.m_volatility_theta.upper,
				DiscreteQuanta,
				theta
			);
			this->m_response.theta.data[thetaHistIndex] += 1;
			this->m_response.theta.sum += theta;
		}

		host_accessor muList(muHists[deviceIndex], read_only);
		for (int index = 0; index < workItems; index++) {
			float mu = muList[index];
			uint32_t muHistIndex = DistributionIndex(
				mu_lower,
				mu_higher,
				DiscreteQuanta,
				mu
			);
			this->m_response.mu.data[muHistIndex] += 1;
			this->m_response.mu.sum += mu;

		}


		host_accessor sigmaList(sigmaHists[deviceIndex], read_only);
		for (int index = 0; index < workItems; index++) {
			float sigma = sigmaList[index];
			uint32_t thetaHistIndex = DistributionIndex(
				this->m_parameter.m_volatility_sigma.lower,
				this->m_parameter.m_volatility_sigma.upper,
				this->m_parameter.m_DiscretCountOfContinuiosSpace,
				sigma
			);
			this->m_response.theta.data[thetaHistIndex] += 1;
			this->m_response.theta.sum += sigma;
		}
	}
	this->iteration_count++;
}