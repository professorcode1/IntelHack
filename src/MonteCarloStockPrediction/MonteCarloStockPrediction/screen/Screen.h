#pragma once
#include "../vendor/iamgui/imgui.h"
#include "../vendor/iamgui/imgui_impl_glfw.h"
#include "../vendor/iamgui/imgui_impl_opengl3.h"
#include "../vendor/iamgui/imspinner.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include "Utils.h"
#include <cpr/cpr.h>
#include <future>
#include <nlohmann/json.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <CL/sycl.hpp>
#include "../Engine/Algorithm.h"

struct FirstScreen {
	char NameToSymbolCSVFile[500];
	char APIKeyFile[500];
	int StockMetricSelectionIndex;

	static char const * const StockMetrics[];
};
struct SecondScreen {
	std::map<std::string, std::string> CompanyNameToSymbol;
	std::string APIKey;
	std::string StockMetricToUse;
};
struct ThirdScreen {
	std::string StockName;
	std::string StockSymbol;
	cpr::AsyncResponse *APICallResponse;
};
struct FourthScreen {
	std::vector<int> preference;
};
enum class ScreenState {
	First,
	Second,
	Third,
	Fourth,
	Fifth,
	Sixth
};
class Screen
{
private:
	FirstScreen firstScreen;
	SecondScreen secondScreen;
	ThirdScreen thirdScreen;
	FourthScreen fourthScreen;
		
	ScreenState screenstate;
	void FirstScreenRender();
	void LoadSecondScreen();
	void SecondScreenRender();
	void LoadThirdScreen(std::map<std::string, std::string>::iterator StockSelected);
	void ThirdScreenRender();
	void LoadFourthScreen(const cpr::Response& response);
	void FourthScreenRender();
	void LoadFifthScreen();
	void FifthScreenRender();
	void LoadSixthScreen();
	void SixthScreenRender();

	std::function<void(boost::gregorian::date, float)> m_populate_Data;
	const std::vector<cl::sycl::device> m_AllDevice;
	std::function<void(std::string, int)> m_populate_DeviceWorkloadPreference;
	std::function<AlgorithmParameter& ()> m_parameterReference;
	std::function<void()> m_initialiseAlgorithm;
	
public:
	Screen(
		const std::function<void(boost::gregorian::date, float)> &populate_Data,
		const std::vector<cl::sycl::device> &AllDevice,
		const std::function<void(std::string, int)>& populate_DeviceWorkloadPreference,
		const std::function<AlgorithmParameter& ()> &parameterReference,
		const std::function<void()> initialiseAlgorithm
	);
	void Render();
};

