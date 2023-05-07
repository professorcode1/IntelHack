#include "ProbabilityDomain.h"



ProbabilityDomain operator*(const ProbabilityDomain &lhs, const float rhs) {
	return {
		lhs.theta * rhs,
		lhs.mu * rhs,
		lhs.sigma * rhs
	};
}
ProbabilityDomain operator*(const float lhs, const ProbabilityDomain& rhs) {
	return rhs * lhs;
}

ProbabilityDomain operator-(const ProbabilityDomain &lhs, const ProbabilityDomain &rhs) {
	return {
		lhs.theta - rhs.theta,
		lhs.mu - rhs.mu,
		lhs.sigma - rhs.sigma
	};
}
ProbabilityDomain operator*(const ProbabilityDomain &lhs, const ProbabilityDomain &rhs) {
	return {
		lhs.theta * rhs.theta,
		lhs.mu * rhs.mu,
		lhs.sigma * lhs.sigma
	};
}
ProbabilityDomain operator+(const ProbabilityDomain& lhs, const ProbabilityDomain& rhs) {
	return {
		lhs.theta + rhs.theta,
		lhs.mu + rhs.mu,
		lhs.sigma + lhs.sigma
	};
}
//extern SYCL_EXTERNAL float sum(const ProbabilityDomain &domain) {
//	return domain.theta + domain.mu + domain.sigma;
//}