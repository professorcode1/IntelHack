#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

std::vector<float> ReadCSVExtractFloatColumns(
	std::string file,
	std::string columnName,
	const int limit = INT32_MAX
);

float geometric_mean(float a, float r, int n);

float evaluate_normal(float x, float mean, float sd);