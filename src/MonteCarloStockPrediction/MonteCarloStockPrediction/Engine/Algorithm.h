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
#include "ProbabilityDomain.h"
#include "types.h"
#include <algorithm>
#include "Utilities.h"
#include <future>
#include <thread>
#define LOG_2_PI_DIVIDED_BY_2 0.9189385332f
#define MINUS_ONE_UPON_SQRT_2_PI -0.3989422804f
class WigginsAlgorithm
{
private:
protected:
	std::future<void> processing;
	const AlgorithmParameter m_parameter;
	AlgorithmResponse m_response;
	int iteration_count;
	cl::sycl::buffer<float,1> m_returns;
	std::vector<std::pair<sycl::queue, AlgorithmDeviceData>> m_QueuesAndData;
	ProbabilityDomain start;

	virtual void BurnIn() = 0;

public:
	WigginsAlgorithm(
		const AlgorithmParameter &parameter,
		const std::vector<float> &stocksdata,
		const std::map<std::string, int> &deviceNameToWorkload
		);

	bool is_completed() ;

	float percent_of_completion() ;

	virtual void iterate() = 0;

	AlgorithmResponse get_response() ;

	//std::vector<std::vector<float> > inference(uint32_t number_of_days, uint32_t number_of_simulations);
	bool IsProcessing();
};

extern SYCL_EXTERNAL float potential_energy_U(
	const ProbabilityDomain domain,
	int timeSeriesLength,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
);

extern SYCL_EXTERNAL float potential_energy_U(
	const ProbabilityDomain domain,
	ProbabilityDomain& differential,
	const AlgorithmParameter& parameters,
	int timeSeriesLength,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
);

extern SYCL_EXTERNAL ProbabilityDomain potential_energy_gradient(
	const ProbabilityDomain domain,
	int tsLength,
	const AlgorithmParameter& parameters,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
);

extern SYCL_EXTERNAL float probability(
	const ProbabilityDomain domain,
	int timeSeriesLength,
	oneapi::dpl::minstd_rand& engine,
	const cl::sycl::accessor<float, 1, sycl::access::mode::read>& returns
);

uint32_t DistributionIndex(float lower, float upper, uint32_t divisions, float value);