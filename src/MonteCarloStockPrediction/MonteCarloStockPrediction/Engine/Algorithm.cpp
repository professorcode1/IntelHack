#include "Algorithm.h"

DiscreteProbabilityDistribution UniformParameters::generateBuffer(
	uint32_t DiscretCountOfContinuiosSpace
) const {
	DiscreteProbabilityDistribution result;
	const float uniform_probability = 1.0f / (this->upper - this->lower);
	std::vector<float> data((size_t)DiscretCountOfContinuiosSpace, uniform_probability);
	float sum = 1.0f;
	result.data = std::move(data);
	result.min = this->lower;
	result.max = this->upper;
	result.sum = sum;
	return result;
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

float NormalParameter::evaluate(float x)const {
	return evaluate_normal(x, this->mean, this->sd);
}
DiscreteProbabilityDistribution NormalParameter::generateBuffer(
	uint32_t DiscretCountOfContinuiosSpace
) const {
	DiscreteProbabilityDistribution result;
	result.data.resize(DiscretCountOfContinuiosSpace);
	result.sum = 0;
	result.min = this->mean - this->buffer_range_sigma_multiplier * this->sd;
	result.max = this->mean + this->buffer_range_sigma_multiplier * this->sd;
	const float delta_x = (result.max - result.min) / DiscretCountOfContinuiosSpace;
	for (int SpaceQuanta = 0; SpaceQuanta < DiscretCountOfContinuiosSpace; SpaceQuanta++) {
		float x = result.min + delta_x * SpaceQuanta;
		float y = this->evaluate(x);
		result.sum += y;
		result.data[SpaceQuanta] = y;
	}
	return result;
}

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

	for (int deviceIndex = 0; deviceIndex < devices.size(); deviceIndex++) {
		std::string deviceName = devices[deviceIndex].get_info<cl::sycl::info::device::name>();
		const int workload = deviceNameToWorkload.at(deviceName);
		workloadTotal += workload;
	}
	if (workloadTotal == 0) {
		throw std::runtime_error("Zero workload selected on all devices");
	}
	for (int deviceIndex = 0; deviceIndex < devices.size(); deviceIndex++) {
		std::string deviceName = devices[deviceIndex].get_info<cl::sycl::info::device::name>();
		const int workload = deviceNameToWorkload.at(deviceName);
		if (workload <= 0)
			continue;
		
		cl::sycl::queue q(devices[deviceIndex]);
		int continuousDiscreteQuanta = this->m_parameter.m_DiscretCountOfContinuiosSpace;
		AlgorithmDeviceData algoDeviceData(
			m_response,
			(float)workload / (float)workloadTotal
		);
		algoDeviceData.m_workload_fraction = workload;
		m_QueuesAndData.push_back(std::make_pair(q, algoDeviceData));
	}

	for (auto QueuesAndDataMemmber : m_QueuesAndData) {
		QueuesAndDataMemmber.second.m_workload_fraction /= workloadTotal;
	}
	this->start = {
		this->m_parameter.m_volatility_theta.testval,
		this->m_parameter.m_volatility_mu.testval,
		this->m_parameter.m_volatility_sigma.testval
	};
	//for(int i=0; i< parameter.m_BurnIn ; i++)
		this->BurnIn();
}	

static int sycl_strogest_cpu_selector(const sycl::device& d) {
	return d.is_cpu();
}


float potential_energy_U(
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

float potential_energy_U(
	const ProbabilityDomain domain,
	ProbabilityDomain &differential,
	const AlgorithmParameter& parameters,
	int timeSeriesLength,
	oneapi::dpl::uniform_real_distribution<float>& dW,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	float U = 0;
	differential.theta = 0;
	differential.mu = 0;
	differential.sigma = 0;
	float x0 = parameters.m_volatility.testval;
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

		differential.theta += del_p_time_del_y_time * (domain.mu - y_time_minus_one);
		differential.mu += del_p_time_del_y_time * domain.theta;
		differential.sigma += del_p_time_del_y_time * dw ;
		U += p_time;
	}
	differential.theta *= -1;
	differential.mu *= -1;
	differential.sigma *= -1;
	return -1 * U;
}

ProbabilityDomain potential_energy_gradient(
	const ProbabilityDomain domain,
	int tsLength,
	const AlgorithmParameter& parameters,
	oneapi::dpl::uniform_real_distribution<float>& dW,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
) {
	ProbabilityDomain gradient;
	potential_energy_U(
		domain, gradient, parameters,  tsLength, dW, engine, returns
	);
	return gradient;
}


bool hmc_sample(
	float epsilon,
	int leapfrog,
	const ProbabilityDomain current_q,
	ProbabilityDomain &new_domain,
	const AlgorithmParameter& parameters,
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
	p = p + (-1.0 * epsilon * potential_energy_gradient(q, tsLength, parameters, dW, engine, returns) * 0.5);
	for (int i = 0; i < leapfrog - 1; i++) {
		q = q + epsilon * p;
		if (q.theta < parameters.m_volatility_theta.lower) {
			q.theta = parameters.m_volatility_theta.lower;
		}
		else if (parameters.m_volatility_theta.upper < q.theta) {
			q.theta = parameters.m_volatility_theta.upper;
		}

		if (q.sigma < parameters.m_volatility_sigma.lower) {
			q.sigma = parameters.m_volatility_sigma.lower;
		}
		else if (parameters.m_volatility_sigma.upper < q.sigma) {
			q.sigma = parameters.m_volatility_sigma.upper;
		}
		p = p + (-1.0 * epsilon * potential_energy_gradient(q, tsLength, parameters, dW, engine, returns));
	}
	p = p + (-1.0 * epsilon * potential_energy_gradient(q, tsLength, parameters, dW, engine, returns) * 2.0f);
	float current_U = potential_energy_U(current_q, tsLength, dW, engine, returns);
	ProbabilityDomain current_K_Domain = current_p * current_p * 0.5;
	float current_K = current_K_Domain.mu + current_K_Domain.sigma + current_K_Domain.theta;
	ProbabilityDomain propsed_K_domain = p * p * 0.5;
	float proposed_U = potential_energy_U(q, tsLength, dW, engine, returns);
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

void WigginsAlgorithm::BurnIn() {
	using namespace cl::sycl;
	queue q(sycl_strogest_cpu_selector);
	unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	buffer<float, 1> result(range<1>{3});
	std::cout << this->m_parameter.m_volatility.dw_lower << "\n" <<
		this->m_parameter.m_volatility.dw_upper << std::endl;
	q.submit([&, this](handler &h) {
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
			oneapi::dpl::uniform_real_distribution<float> dW(parameter.m_volatility.dw_lower, parameter.m_volatility.dw_upper);
			ProbabilityDomain oldDomain{start}, nextDomain;
			for (int i = 0; i < parameter.m_BurnIn; i++) {
				hmc_sample(
					parameter.m_epsilon, parameter.m_leapfrog,
					oldDomain,
					nextDomain,
					parameter,
					theta_normal,
					mu_normal,
					sigma_normal,
					zero_one_uniform,
					tsLength,
					dW,
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
	std::cout << "Burn In Done, result " << std::endl;
	std::cout << this->start.theta << std::endl;
	std::cout << this->start.mu << std::endl;
	std::cout << this->start.sigma << std::endl;
}

bool WigginsAlgorithm::is_completed() {
	const float number_of_iteration = 
		this->m_parameter.m_MCMCIteration / this->m_parameter.m_GraphUpdateIteration
		+ (this->m_parameter.m_MCMCIteration % this->m_parameter.m_GraphUpdateIteration > 0);
	return this->iteration_count >= number_of_iteration;
}

uint32_t DistributionIndex(float lower, float upper, uint32_t divisions, float value) {
	float delx = (upper - lower) / divisions;
	uint32_t res = floor((value - lower) / delx);
	return res;
}

void sort_buffer(cl::sycl::handler& h, cl::sycl::accessor<uint32_t, 1,cl::sycl::access_mode::write> &buffer) {
	int buffer_size = buffer.get_size();
	for (int sort_step = 0; sort_step <= buffer_size; sort_step++) {
		//<= to avoid risk. 
		h.parallel_for(cl::sycl::range<1>((buffer_size - 1) / 2), [=](cl::sycl::id<1> thread_id) {
			int i = 1 + thread_id * 2;
			if (buffer[i] > buffer[i + 1]) {
				std::swap(buffer[i], buffer[i + 1]);
			}
		});
		h.parallel_for(cl::sycl::range<1>((buffer_size - 1) / 2), [=](cl::sycl::id<1> thread_id) {
			int i = thread_id * 2;
			if (buffer[i] > buffer[i + 1]) {
				std::swap(buffer[i], buffer[i + 1]);
			}
		});
	}
}
void fill_buffer(cl::sycl::handler& h, cl::sycl::buffer<uint32_t, 1>& buffer, uint32_t fillValue) {
	using namespace cl::sycl;

	accessor Accessor(buffer, h, write_only);
	h.parallel_for(range<1>(buffer.size()), [=](id<1> thread_id) {
		Accessor[thread_id] = fillValue;
	});
}
cl::sycl::event exectue_wiggins_algorithm_on_device(
	cl::sycl::queue &q, const AlgorithmDeviceData &device_data,
	const AlgorithmParameter &parameter, const ProbabilityDomain &start,
	 cl::sycl::buffer<float,1> returns
){
	using namespace cl::sycl;
	const int workItems = ceil(parameter.m_GraphUpdateIteration * device_data.m_workload_fraction);
	buffer<uint32_t, 1> theta(workItems);
	buffer<uint32_t, 1> mu(workItems);
	buffer<uint32_t, 1> sigma(workItems);
	const uint32_t quantisation = parameter.m_DiscretCountOfContinuiosSpace;

	buffer<uint32_t, 1> delta_theta(quantisation);
	buffer<uint32_t, 1> delta_mu(quantisation);
	buffer<uint32_t, 1> delta_sigma(quantisation);
	 
	const buffer<float, 1>& deviceThetaHist = device_data.theta.data;
	const buffer<float, 1>& deviceMuHist = device_data.mu.data;
	const buffer<float, 1>& deviceSigmaHist= device_data.sigma.data;
	
	buffer<uint32_t, 1> probabilityDomainSum(3);

	int tsLength = returns.size();
	unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
	return q.submit([&](handler& h) {
		accessor thetaAccessor(theta, h, write_only);
		accessor muAccessor(mu, h, write_only);
		accessor sigmaAccessor(sigma, h, write_only);
		accessor returnsAccessor(returns, h, read_only);
		accessor probabilityDomainAccessor(probabilityDomainSum, h, write_only, no_init);

		h.parallel_for(range<1>(workItems), [=](id<1> thread_id) {
			oneapi::dpl::minstd_rand engine(seed, thread_id);
			oneapi::dpl::normal_distribution<float> theta_normal(0.0f, parameter.m_volatility_theta.guassian_step_sd);
			oneapi::dpl::normal_distribution<float> mu_normal(0.0f, parameter.m_volatility_mu.guassian_step_sd);
			oneapi::dpl::normal_distribution<float> sigma_normal(0.0f, parameter.m_volatility_sigma.guassian_step_sd);
			oneapi::dpl::uniform_real_distribution<float> zero_one_uniform(0.0f, 1.0f);
			oneapi::dpl::uniform_real_distribution<float> dW(parameter.m_volatility.dw_lower, parameter.m_volatility.dw_upper);
			ProbabilityDomain oldDomain{ start }, nextDomain;
			hmc_sample(
				parameter.m_epsilon, parameter.m_leapfrog,
				oldDomain,
				nextDomain,
				parameter,
				theta_normal,
				mu_normal,
				sigma_normal,
				zero_one_uniform,
				tsLength,
				dW,
				engine,
				returnsAccessor
			);
			thetaAccessor[thread_id] = DistributionIndex(
				parameter.m_volatility_theta.lower,
				parameter.m_volatility_theta.upper,
				parameter.m_DiscretCountOfContinuiosSpace,
				nextDomain.theta
			);
			float mu_lower = parameter.m_volatility_mu.mean - parameter.m_volatility_mu.buffer_range_sigma_multiplier * parameter.m_volatility_mu.sd;
			float mu_upper = parameter.m_volatility_mu.mean + parameter.m_volatility_mu.buffer_range_sigma_multiplier * parameter.m_volatility_mu.sd;
			muAccessor[thread_id] = DistributionIndex(
				mu_lower,
				mu_upper,
				parameter.m_DiscretCountOfContinuiosSpace,
				nextDomain.mu
			);
			sigmaAccessor[thread_id] = DistributionIndex(
				parameter.m_volatility_sigma.lower,
				parameter.m_volatility_sigma.upper,
				parameter.m_DiscretCountOfContinuiosSpace,
				nextDomain.sigma
			); ;
		});
		sort_buffer(h, thetaAccessor);
		sort_buffer(h, muAccessor);
		sort_buffer(h, sigmaAccessor);
		fill_buffer(h, delta_theta, 0);
		fill_buffer(h, delta_mu, 0);
		fill_buffer(h, delta_sigma, 0);
		accessor delta_thetaAccessor(delta_theta, h, write_only);
		accessor delta_muAccessor(delta_mu, h, write_only);
		accessor delta_sigmaAccessor(delta_sigma, h, write_only);

		accessor thetaHistAccessor(deviceThetaHist, h, read_write);
		const int maximum_binary_search_iterations = ceil(log2(workItems)) + 1;//+1 to avoid risk
		//better a little slow than 5 hours debugging
		h.parallel_for(range<1>(quantisation), [=](id<1> thread_id) {
			int low = 0, high = workItems - 1, first = -1 , last = -1;
			for (int binary_search_counter = 0; binary_search_counter < maximum_binary_search_iterations; binary_search_counter++) {
				int mid = (low + high) / 2;

				if (thetaAccessor[mid] > thread_id)
					high = mid - 1;
				else if (thetaAccessor[mid] < thread_id)
					low = mid + 1;
				else {
					first = mid;
					high = mid - 1;
				}
			}
		
			low = 0, high = workItems - 1;
			for(int binary_search_counter = 0; binary_search_counter < maximum_binary_search_iterations; binary_search_counter++) {
				int mid = (low + high) / 2;

				if (thetaAccessor[mid] > thread_id)
					high = mid - 1;
				else if (thetaAccessor[mid] < thread_id)
					low = mid + 1;

				else {
					last = mid;
					low = mid + 1;
				}
			}
			
			if (first != -1) {
				delta_thetaAccessor[thread_id] = last - first + 1;
			}
			
		});

		h.parallel_for(range<1>(quantisation), [=](id<1> thread_id) {
			int low = 0, high = workItems - 1, first = -1, last = -1;
			for (int binary_search_counter = 0; binary_search_counter < maximum_binary_search_iterations; binary_search_counter++) {
				int mid = (low + high) / 2;

				if (muAccessor[mid] > thread_id)
					high = mid - 1;
				else if (muAccessor[mid] < thread_id)
					low = mid + 1;
				else {
					first = mid;
					high = mid - 1;
				}
			}

			low = 0, high = workItems - 1;
			for (int binary_search_counter = 0; binary_search_counter < maximum_binary_search_iterations; binary_search_counter++) {
				int mid = (low + high) / 2;

				if (muAccessor[mid] > thread_id)
					high = mid - 1;
				else if (muAccessor[mid] < thread_id)
					low = mid + 1;

				else {
					last = mid;
					low = mid + 1;
				}
			}

			if (first != -1) {
				delta_muAccessor[thread_id] = last - first + 1;
			}

		});

		h.parallel_for(range<1>(quantisation), [=](id<1> thread_id) {
			int low = 0, high = workItems - 1, first = -1, last = -1;
			for (int binary_search_counter = 0; binary_search_counter < maximum_binary_search_iterations; binary_search_counter++) {
				int mid = (low + high) / 2;

				if (sigmaAccessor[mid] > thread_id)
					high = mid - 1;
				else if (sigmaAccessor[mid] < thread_id)
					low = mid + 1;
				else {
					first = mid;
					high = mid - 1;
				}
			}

			low = 0, high = workItems - 1;
			for (int binary_search_counter = 0; binary_search_counter < maximum_binary_search_iterations; binary_search_counter++) {
				int mid = (low + high) / 2;

				if (sigmaAccessor[mid] > thread_id)
					high = mid - 1;
				else if (sigmaAccessor[mid] < thread_id)
					low = mid + 1;

				else {
					last = mid;
					low = mid + 1;
				}
			}

			if (first != -1) {
				delta_sigmaAccessor[thread_id] = last - first + 1;
			}
			});

		h.parallel_for(range<1>(quantisation), [=](id<1> thread_id) {
				
		});

		h.parallel_for(range<1>(quantisation), [=](id<1> thread_id) {
			auto theta = sycl::ext::oneapi::atomic_ref<
				uint32_t, sycl::ext::oneapi::memory_order::relaxed,
				sycl::ext::oneapi::memory_scope::device,
				sycl::access::address_space::global_space>(probabilityDomainAccessor[0]);
			theta.fetch_add(delta_thetaAccessor[thread_id]);

			auto mu = sycl::ext::oneapi::atomic_ref<
				uint32_t, sycl::ext::oneapi::memory_order::relaxed,
				sycl::ext::oneapi::memory_scope::device,
				sycl::access::address_space::global_space>(probabilityDomainAccessor[1]);
			mu.fetch_add(delta_muAccessor[thread_id]);

			auto sigma = sycl::ext::oneapi::atomic_ref<
				uint32_t, sycl::ext::oneapi::memory_order::relaxed,
				sycl::ext::oneapi::memory_scope::device,
				sycl::access::address_space::global_space>(probabilityDomainAccessor[2]);
			sigma.fetch_add(delta_sigmaAccessor[thread_id]);
		});
	});
}

void WigginsAlgorithm::iterate() {
	std::vector<cl::sycl::event> events;
	for (int deviceIndex = 0; deviceIndex < this->m_QueuesAndData.size(); deviceIndex++) {
		events.push_back(
			exectue_wiggins_algorithm_on_device(
				this->m_QueuesAndData[deviceIndex].first,
				this->m_QueuesAndData[deviceIndex].second,
				this->m_parameter,this->start,
				this->m_returns
			)
		);
	}
}