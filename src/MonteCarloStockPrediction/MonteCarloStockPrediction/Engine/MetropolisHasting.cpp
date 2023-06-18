#include "MetropolisHasting.h"

MetropolisHasting::MetropolisHasting(
	const AlgorithmParameter& parameter,
	const std::vector<float>& stocksdata,
	const std::map<std::string, int>& deviceNameToWorkload
) :WigginsAlgorithm{ parameter ,stocksdata , deviceNameToWorkload } {
	this->BurnIn();
}

static int sycl_strogest_cpu_selector(const sycl::device& d) {
	return d.is_cpu();
}

bool mh_sample(
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
	ProbabilityDomain p{ theta_normal(engine) , mu_normal(engine), sigma_normal(engine) };
	ProbabilityDomain q = current_q + p;
	const float current_f = probability(current_q, tsLength, engine, returns);
	const float purposed_f = probability(q, tsLength, engine, returns);
	const bool accepted = (purposed_f / current_f) > zero_one_uniform(engine);
	new_domain = ((int)accepted) * q + ((int)(1 ^ accepted)) * current_q;
	return accepted;
}

void MetropolisHasting::BurnInInternal() {
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
				mh_sample(
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

static cl::sycl::event exectue_wiggins_algorithm_on_device(
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
			mh_sample(
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

void MetropolisHasting::iterateInternal() {
	
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

void MetropolisHasting::BurnIn() {
	this->processing = std::async(std::launch::async, &MetropolisHasting::BurnInInternal, this);
}
void MetropolisHasting::iterate() {
	this->processing = std::async(std::launch::async, &MetropolisHasting::iterateInternal, this);
}