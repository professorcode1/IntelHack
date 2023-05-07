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


class WigginsAlgorithm
{
private:
	const AlgorithmParameter m_parameter;
	AlgorithmResponse m_response;
	int iteration_count;
	cl::sycl::buffer<float,1> m_returns;
	std::vector<std::pair<sycl::queue, AlgorithmDeviceData>> m_QueuesAndData;
	ProbabilityDomain start;
	void BurnIn();

public:
	WigginsAlgorithm(
		const AlgorithmParameter &parameter,
		const std::vector<float> &stocksdata,
		const std::map<std::string, int> &deviceNameToWorkload
		);

	bool is_completed();

	void iterate();
};