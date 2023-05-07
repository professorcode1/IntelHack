#pragma once
#include <oneapi/dpl/random>
#include <stdint.h>
#include <vector>
#include <sycl/sycl.hpp>
#define _USE_MATH_DEFINES
#include <math.h>
#include <map>	
#include <unordered_map>
#include <chrono>
#include <CL/sycl.hpp>
#include "ProbabilityDomain.h"
#include "types.h"


class WigginsAlgorithm
{
private:
	const AlgorithmParameter m_parameter;
	AlgorithmResponse response;
	int iteration_count;
	cl::sycl::buffer<float,1> m_returns;
	std::unordered_map<sycl::queue, AlgorithmDeviceData> m_QueuesAndData;
	float potential_energy_U(
		const ProbabilityDomain domain,
		int timeSeriesLength,
		oneapi::dpl::uniform_real_distribution<float> &dW,
		oneapi::dpl::minstd_rand &engine, 
		const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
	);

	float potential_energy_U(
		const ProbabilityDomain domain,
		ProbabilityDomain& differential,
		int timeSeriesLength,
		oneapi::dpl::uniform_real_distribution<float>& dW,
		oneapi::dpl::minstd_rand& engine,
		const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
	);

	ProbabilityDomain potential_energy_gradient(
		const ProbabilityDomain domain,
		int timeSeriesLength,
		oneapi::dpl::uniform_real_distribution<float>& dW,
		oneapi::dpl::minstd_rand& engine,
		const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
	);

	bool hmc_sample(
		float epsilon,
		int leapfrog,
		const ProbabilityDomain current_q,
		ProbabilityDomain& new_domain,
		oneapi::dpl::normal_distribution<float>& theta_normal,
		oneapi::dpl::normal_distribution<float>& mu_normal,
		oneapi::dpl::normal_distribution<float>& sigma_normal,
		oneapi::dpl::uniform_real_distribution<float>& zero_one_uniform,
		int timeSeriesLength,
		oneapi::dpl::uniform_real_distribution<float>& dW,
		oneapi::dpl::minstd_rand& engine,
		const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
	);

	void BurnIn();

public:
	WigginsAlgorithm(
		const AlgorithmParameter &parameter,
		const std::vector<float> &stocksdata,
		const std::map<std::string, int> &deviceNameToWorkload
		);

	std::optional<AlgorithmResponse> next();

};

