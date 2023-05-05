#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

std::vector<std::string> string_split(std::string& line, std::string delimiter);
std::vector<std::string> ReadCSVExtractStringColumns(
	std::string file,
	std::string columnName,
	const int limit = std::numeric_limits<int>::max()
);
