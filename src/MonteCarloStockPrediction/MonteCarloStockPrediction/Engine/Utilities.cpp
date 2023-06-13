#include "Utilities.h"

static std::vector<std::string> string_split(std::string& line, std::string delimiter) {
	using namespace std;
	vector<string> split;
	size_t pos = 0;
	while ((pos = line.find(delimiter)) != string::npos) {
		split.push_back(line.substr(0, pos));
		line.erase(0, pos + delimiter.length());
	}
	split.push_back(line);
	return split;
}

std::vector<float> ReadCSVExtractFloatColumns(
	std::string file,
	std::string columnName,
	const int limit 
) {
	using namespace std;
	vector<float> result;
	string line;
	ifstream fileStream(file);
	getline(fileStream, line);
	const auto header = string_split(line, ",");
	const auto columnIterator = find(header.begin(), header.end(), columnName);
	const int columnIndex = columnIterator - header.begin();
	int lines_read = 0;
	while (getline(fileStream, line) && lines_read < limit) {
		result.push_back(
			stof(
				string_split(line, ",")[columnIndex]
			)
		);
		lines_read++;
	}
	return result;
}

float geometric_mean(float a, float r, int n) {
	float num = a * (std::pow(r, n) - 1);
	float den = r - 1;
	return num / den;
}

float evaluate_normal(float x, float mean, float sd) {
	x = x - mean;
	x /= sd;
	x *= x;
	x *= -0.5f;
	x = exp(x);
	x /= sd;
	x /= sqrtf(2 * M_PI);
	return x;
}