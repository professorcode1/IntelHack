#include "Algorithm.h"

std::pair<std::vector<float>, float> UniformParameters::generateBuffer(
	uint32_t DiscretCountOfContinuiosSpace
) {
	const float uniform_probability = 1.0f / (this->upper - this->lower);
	std::vector<float> data((size_t)DiscretCountOfContinuiosSpace, uniform_probability);
	float sum = 1.0f;
	return std::make_pair(data, sum);
}
float NormalParameter::evaluate(float x) {
	x = x - this->mean;
	x /= this->sd;
	x *= x;
	x *= -0.5f;
	x = exp(x);
	x /= this->sd;
	x /= sqrtf(2 * M_PI);
	return x;
}
std::pair<std::vector<float>, float> NormalParameter::generateBuffer(
	uint32_t DiscretCountOfContinuiosSpace
) {
	std::vector<float> data(DiscretCountOfContinuiosSpace);
	float sum = 0;
	const float start = this->mean - this->buffer_range_sigma_multiplier * this->sd;
	const float end = this->mean + this->buffer_range_sigma_multiplier * this->sd;
	const float delta_x = (end - start) / DiscretCountOfContinuiosSpace;
	for (int SpaceQuanta = 0; SpaceQuanta < DiscretCountOfContinuiosSpace; SpaceQuanta++) {
		float x = start + delta_x * SpaceQuanta;
		float y = this->evaluate(x);
		sum += y;
		data[SpaceQuanta] = y;
	}
	return std::make_pair(data, sum);
}

Algorithm::Algorithm(
	const AlgorithmParameter& parameter,
	const std::vector<float>& stocksdata,
	const std::map<std::string, int>& deviceNameToWorkload
):
	m_parameter{ parameter },
	m_stocksdata{ stocksdata }
{
	this->iteration_count = 0;
}	
