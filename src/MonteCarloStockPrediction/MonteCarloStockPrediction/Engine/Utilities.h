#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

std::vector<float> ReadCSVExtractFloatColumns(
	std::string file,
	std::string columnName,
	const int limit = std::numeric_limits<int>::max()
);

float geometric_mean(float a, float r, int n);