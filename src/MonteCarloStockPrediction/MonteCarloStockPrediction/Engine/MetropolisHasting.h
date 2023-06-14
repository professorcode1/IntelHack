#pragma once
#include "Algorithm.h"
class MetropolisHasting:public WigginsAlgorithm
{
private:
	void BurnIn() override;
public:
	MetropolisHasting(
		const AlgorithmParameter& parameter,
		const std::vector<float>& stocksdata,
		const std::map<std::string, int>& deviceNameToWorkload
	);


	void iterate() override;

};

