#include "types.h"

DiscreteProbabilityDistribution UniformParameters::generateBuffer(
	uint32_t DiscretCountOfContinuiosSpace
) const {
	DiscreteProbabilityDistribution result;
	const float uniform_probability = 1.0f / (this->upper - this->lower);
	std::vector<float> data((size_t)DiscretCountOfContinuiosSpace, uniform_probability);
	float sum = 1.0f;
	result.data = std::move(data);
	result.min = this->lower;
	result.max = this->upper;
	result.sum = sum;
	return result;
}

float NormalParameter::evaluate(float x)const {
	return evaluate_normal(x, this->mean, this->sd);
}
DiscreteProbabilityDistribution NormalParameter::generateBuffer(
	uint32_t DiscretCountOfContinuiosSpace
) const {
	DiscreteProbabilityDistribution result;
	result.data.resize(DiscretCountOfContinuiosSpace);
	result.sum = 0;
	result.min = this->mean - this->buffer_range_sigma_multiplier * this->sd;
	result.max = this->mean + this->buffer_range_sigma_multiplier * this->sd;
	const float delta_x = (result.max - result.min) / DiscretCountOfContinuiosSpace;
	for (int SpaceQuanta = 0; SpaceQuanta < DiscretCountOfContinuiosSpace; SpaceQuanta++) {
		float x = result.min + delta_x * SpaceQuanta;
		float y = this->evaluate(x);
		result.sum += y;
		result.data[SpaceQuanta] = y;
	}
	return result;
}
