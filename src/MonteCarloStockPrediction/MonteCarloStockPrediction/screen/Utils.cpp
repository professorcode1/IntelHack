#include "Utils.h"

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

std::vector<std::string> ReadCSVExtractStringColumns(
	std::string file,
	std::string columnName,
	const int limit
) {
	using namespace std;
	vector<string> result;
	string line;
	ifstream fileStream(file);
	if (fileStream.fail()) {
		throw std::runtime_error(file + "cannot be opened");
	}
	getline(fileStream, line);
	const auto header = string_split(line, ",");
	const auto columnIterator = find(header.begin(), header.end(), columnName);
	const int columnIndex = columnIterator - header.begin();
	int lines_read = 0;
	while (getline(fileStream, line) && lines_read < limit) {
		result.push_back(
				string_split(line, ",").at(columnIndex)
		);
		lines_read++;
	}
	return result;
}
