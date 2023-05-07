#include "ProbabilityDomain.h"

ProbabilityDomain ProbabilityDomain::operator+(const ProbabilityDomain& other) {
	ProbabilityDomain res;
	res.theta = this->theta + other.theta;
	res.mu = this->mu + other.mu;
	res.sigma = this->sigma + other.sigma;
	return res;
}


ProbabilityDomain operator*(const ProbabilityDomain lhs, const float rhs) {
	ProbabilityDomain res = lhs;
	res.theta *= rhs;
	res.mu *= rhs;
	res.sigma *= rhs;
	return res;
}
ProbabilityDomain operator*(const float lhs, const ProbabilityDomain& rhs) {
	return rhs * lhs;
}

ProbabilityDomain operator-(const ProbabilityDomain lhs, const ProbabilityDomain rhs) {
	return {
		lhs.theta - rhs.theta,
		lhs.mu - rhs.mu,
		lhs.sigma - rhs.sigma
	};
}
ProbabilityDomain operator*(const ProbabilityDomain lhs, const ProbabilityDomain rhs) {
	return {
		lhs.theta * rhs.theta,
		lhs.mu * rhs.mu,
		lhs.sigma * lhs.sigma
	};
}
float ProbabilityDomain::sum() {
	return this->theta + this->mu + this->sigma;
}