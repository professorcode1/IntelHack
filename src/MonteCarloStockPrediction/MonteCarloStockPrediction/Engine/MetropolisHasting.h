#pragma once
#include "Algorithm.h"
class MetropolisHasting:public WigginsAlgorithm
{
private:
	void BurnIn() override;
	void BurnInInternal();
public:
	MetropolisHasting(
		const AlgorithmParameter& parameter,
		const std::vector<float>& stocksdata,
		const std::map<std::string, int>& deviceNameToWorkload
	);
	
	void iterateInternal();
	void iterate() override;

};

