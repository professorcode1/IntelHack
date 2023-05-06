#pragma once
#include <stdint.h>
typedef struct UniformParameters {
	float lower;
	float upper;
	float testval;
	float guassian_step_sd;
} UniformParameters;

typedef struct NormalParameter {
	float mean;
	float sd;
	float testval;
	float guassian_step_sd;
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
} AlgorithmParameter;

class Algorithm
{
private:
	AlgorithmParameter parameter;

public:
	Algorithm();
	AlgorithmParameter& getParameterReference() {
		return this->parameter;
	}
	friend class Screen;
};

