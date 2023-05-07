#pragma once
#include <vector>
#include <CL/sycl.hpp>

typedef struct UniformParameters {
	float lower;
	float upper;
	float testval;
	float guassian_step_sd;

	std::pair<std::vector<float>, float> generateBuffer(uint32_t DiscretCountOfContinuiosSpace);
} UniformParameters;

typedef struct NormalParameter {
	float mean;
	float sd;
	float testval;
	float guassian_step_sd;
	uint32_t buffer_range_sigma_multiplier;

	float evaluate(float x);
	std::pair<std::vector<float>, float> generateBuffer(uint32_t DiscretCountOfContinuiosSpace);
} NormalParameter;

typedef struct EulerMaruyama {
	float dt;
	float testval;
	float dw_lower;
	float dw_upper;
} EulerMaruyama;

typedef struct AlgorithmParameter {
	UniformParameters m_volatility_theta;
	NormalParameter m_volatility_mu;
	UniformParameters m_volatility_sigma;
	EulerMaruyama m_volatility;
	uint32_t m_MCMCIteration;
	uint32_t m_GraphUpdateIteration;
	uint32_t m_NumberOfDaysToUse;
	uint32_t m_BurnIn;
	uint32_t m_DiscretCountOfContinuiosSpace;
	uint32_t m_leapfrog;
	float m_epsilon;
} AlgorithmParameter;

typedef struct DiscreteProbabilityDistribution {
	std::vector<float> data;
	float min;
	float max;
	float sum;
} DiscreteProbabilityDistribution;

typedef struct DiscreteProbabilityDistribution_device {
	cl::sycl::buffer<float> data;
	float min;
	float max;
	float sum;

	DiscreteProbabilityDistribution_device(DiscreteProbabilityDistribution dist):
		data{dist.data}
	{
		this->max = dist.max;
		this->min = dist.min;
		this->sum = dist.sum;
	}
} DiscreteProbabilityDistribution_device;


typedef struct AlgorithmResponse {
	DiscreteProbabilityDistribution theta;
	DiscreteProbabilityDistribution mu;
	DiscreteProbabilityDistribution sigma;
} AlgorithmResponse;

typedef struct AlgorithmDeviceData {
	float m_workload_fraction;
} AlgorithmDeviceData;