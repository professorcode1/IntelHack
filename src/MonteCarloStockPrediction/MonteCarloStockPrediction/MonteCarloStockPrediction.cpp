#include <iostream>

#include "SYCLComputer.h"
#include "Utilities.h"

int main() {
	SYCLComputer SyclComputer;
	auto vec = std::vector<float>({ 1,4,9,16,25 });
	const auto StockAdjClose = ReadCSVExtractFloatColumns(
		"C:\\Users\\raghk\\OneDrive\\Documents\\hack\\intel mc\\data\\stocks\\Russell2000.csv", 
		"Adj Close", 
		10);
	const std::pair<float, float> MuAndSigma = SyclComputer.StockMuAndSigma(StockAdjClose, 1 - (2.0 / 31.0));
	std::cout << MuAndSigma.first << "\t"<< MuAndSigma.second << std::endl;
	const auto prediction = SyclComputer.predict(
		MuAndSigma.first,
		MuAndSigma.second,
		StockAdjClose.back(),
		5,
		5
	);
	std::cout << "Starting price\t" << StockAdjClose.back() << std::endl;
	std::cout << " One of the predicitons\n";
	std::copy(prediction[0].begin(), prediction[0].end(), std::ostream_iterator<float>(std::cout, " "));
	std::cout << std::endl;
	return 0;
}