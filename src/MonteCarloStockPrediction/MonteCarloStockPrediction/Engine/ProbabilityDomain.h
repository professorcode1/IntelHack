#pragma once
#include<CL/sycl.hpp>

extern SYCL_EXTERNAL struct ProbabilityDomain {
	float theta;
	float mu;
	float sigma;
} ;
typedef struct ProbabilityDomain ProbabilityDomain;
//extern SYCL_EXTERNAL float sum(ProbabilityDomain domain);
extern SYCL_EXTERNAL ProbabilityDomain operator*(const ProbabilityDomain &lhs, const float rhs);
extern SYCL_EXTERNAL ProbabilityDomain operator*(const float lhs, const ProbabilityDomain& rhs);
extern SYCL_EXTERNAL ProbabilityDomain operator-(const ProbabilityDomain &lhs, const ProbabilityDomain &rhs);
extern SYCL_EXTERNAL ProbabilityDomain operator*(const ProbabilityDomain& lhs, const ProbabilityDomain& rhs);
extern SYCL_EXTERNAL ProbabilityDomain operator+(const ProbabilityDomain &lhs, const ProbabilityDomain &rhs);
