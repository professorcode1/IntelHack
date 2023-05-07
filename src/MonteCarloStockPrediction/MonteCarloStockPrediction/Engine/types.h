#pragma once
#include <vector>
#include <CL/sycl.hpp>

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
	DiscreteProbabilityDistribution_device() = delete;
	DiscreteProbabilityDistribution_device(const DiscreteProbabilityDistribution& dist) :
		data{ dist.data } {
		this->max = dist.max;
		this->min = dist.min;
		this->sum = dist.sum;
	}
} DiscreteProbabilityDistribution_device;

typedef struct UniformParameters {
	float lower;
	float upper;
	float testval;
	float guassian_step_sd;

	DiscreteProbabilityDistribution generateBuffer(uint32_t DiscretCountOfContinuiosSpace) const;
} UniformParameters;

typedef struct NormalParameter {
	float mean;
	float sd;
	float testval;
	float guassian_step_sd;
	uint32_t buffer_range_sigma_multiplier;

	float evaluate(float x) const;
	DiscreteProbabilityDistribution generateBuffer(uint32_t DiscretCountOfContinuiosSpace) const;
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




typedef struct AlgorithmResponse {
	DiscreteProbabilityDistribution theta;
	DiscreteProbabilityDistribution mu;
	DiscreteProbabilityDistribution sigma;
	AlgorithmResponse() = delete;
	AlgorithmResponse(
		const DiscreteProbabilityDistribution& t,
		const DiscreteProbabilityDistribution& m,
		const DiscreteProbabilityDistribution& s
	) :theta{ t }, mu{ m }, sigma{ s } {};
} AlgorithmResponse;

typedef struct AlgorithmDeviceData {
	const float m_workload_fraction;
	DiscreteProbabilityDistribution_device theta;
	DiscreteProbabilityDistribution_device mu;
	DiscreteProbabilityDistribution_device sigma;

	AlgorithmDeviceData(
		const DiscreteProbabilityDistribution& t,
		const DiscreteProbabilityDistribution& m,
		const DiscreteProbabilityDistribution& s,
		float workload_fraction
	) :theta{ t }, mu{ m }, sigma{ s }, m_workload_fraction{ workload_fraction } {};

	AlgorithmDeviceData(
		const AlgorithmResponse& algorithmRes,
		float workload_fraction
	) :theta{ algorithmRes.theta }, mu{ algorithmRes.mu }, sigma{ algorithmRes.sigma }, m_workload_fraction{ workload_fraction } {};
} AlgorithmDeviceData;