#include "Algorithm.h"

Algorithm::Algorithm() {


	this->parameter.m_MCMCIteration = 3000;
	this->parameter.m_GraphUpdateIteration = 100;
	this->parameter.m_NumberOfDaysToUse = 365;
	this->parameter.m_BurnIn = 1000;
	this->parameter.m_DiscretCountOfContinuiosSpace = 100;

	this->parameter.m_volatility_theta.lower = 0.0;
	this->parameter.m_volatility_theta.upper = 1.0;
	this->parameter.m_volatility_theta.testval = 0.5;
	this->parameter.m_volatility_theta.guassian_step_sd = 1.0f / (4.0 * 6.0);

	this->parameter.m_volatility_mu.mean = -5.0f;
	this->parameter.m_volatility_mu.sd = 0.1f;
	this->parameter.m_volatility_mu.testval = -5.0f;
	this->parameter.m_volatility_mu.guassian_step_sd = 0.1f / 4.0f;

	this->parameter.m_volatility_sigma.lower = 0.001f;
	this->parameter.m_volatility_sigma.upper = 0.2f;
	this->parameter.m_volatility_sigma.testval = 0.05f;
	this->parameter.m_volatility_sigma.guassian_step_sd = (0.02 - 0.001) / (4.0 * 6.0);

	this->parameter.m_volatility.dt = 1.0f;
	this->parameter.m_volatility.testval = 1.0f;
	this->parameter.m_volatility.dw_lower = 0.0f;
	this->parameter.m_volatility.dw_upper = 0.1f;
}	
