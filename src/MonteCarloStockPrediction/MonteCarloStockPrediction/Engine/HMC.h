#pragma once
#include "Algorithm.h"
class HMC:public WigginsAlgorithm
{
private:
	void BurnInInternal();
	void BurnIn() override;
public:
	HMC(
		const AlgorithmParameter& parameter,
		const std::vector<float>& stocksdata,
		const std::map<std::string, int>& deviceNameToWorkload
	);

	void iterateInternal();
	void iterate() override;
};

