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
	/*cl::sycl::event run_inference(
		cl::sycl::queue& q, uint32_t number_of_simulations,
		uint32_t number_of_days, cl::sycl::buffer<float, 2> data);*/

public:
	WigginsAlgorithm(
		const AlgorithmParameter &parameter,
		const std::vector<float> &stocksdata,
		const std::map<std::string, int> &deviceNameToWorkload
		);

	bool is_completed();

	float percent_of_completion();

	void iterate();

	AlgorithmResponse get_response();

	//std::vector<std::vector<float> > inference(uint32_t number_of_days, uint32_t number_of_simulations);
};