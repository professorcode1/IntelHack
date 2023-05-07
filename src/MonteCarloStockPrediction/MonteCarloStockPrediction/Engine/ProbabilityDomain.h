#pragma once

typedef struct ProbabilityDomain {
	float theta;
	float mu;
	float sigma;

	ProbabilityDomain operator+(const ProbabilityDomain& other);
	float sum();
} ProbabilityDomain;

ProbabilityDomain operator*(const ProbabilityDomain lhs, const float rhs);
ProbabilityDomain operator*(const float lhs, const ProbabilityDomain& rhs);
ProbabilityDomain operator-(const ProbabilityDomain lhs, const ProbabilityDomain rhs);
ProbabilityDomain operator*(const ProbabilityDomain lhs, const ProbabilityDomain rhs);
