#pragma once
#include "../vendor/iamgui/imgui.h"
#include "../vendor/iamgui/imgui_impl_glfw.h"
#include "../vendor/iamgui/imgui_impl_opengl3.h"
#include "../vendor/iamgui/imspinner.h"
#include "../vendor/pbPlots/pbPlots.h"
#include "../vendor/pbPlots/supportLib.h"
#include "Utils.h"
#include "../Engine/types.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <cpr/cpr.h>
#include <future>
#include <nlohmann/json.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <CL/sycl.hpp>
#include <GLFW/glfw3.h>

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

struct SixthScreen {
	uint32_t number_of_days_to_simulate = 10;
	uint32_t number_of_simulations_to_run = 100;
};
enum class ScreenState {
	First,
	Second,
	Third,
	Fourth,
	Fifth,
	Sixth,
	Seventh,
};
class Screen
{
private:
	FirstScreen firstScreen;
	SecondScreen secondScreen;
	ThirdScreen thirdScreen;
	FourthScreen fourthScreen;
	SixthScreen sixthScreen;
		
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
	void LoadSeventhScreen();
	void SeventhScreenRender();


	void generateLargeStockPlot(
		const std::map<boost::gregorian::date, float>& dateMap,
		const std::string& stockName
	);

	std::function<void(boost::gregorian::date, float)> m_populate_Data;
	const std::vector<cl::sycl::device> m_AllDevice;
	std::function<void(std::string, int)> m_populate_DeviceWorkloadPreference;
	std::function<AlgorithmParameter& ()> m_parameterReference;
	std::function<void()> m_initialiseAlgorithm;

	std::function<void()> m_algorithmIterate;
	std::function<float()> m_algorithmCompletionPercent;
	std::function<bool()> m_algorithmCompleted;
	std::function<AlgorithmResponse()> m_algorithmResonse;

	unsigned char* largeStocksPlot;
	ScatterPlotSettings* stocksSettings;
	
	unsigned char* algorithm_progress_screen;
	void generateAlgorithmProgressPage();
	void updateAlgorithmProgressPage();
	unsigned char* genereateHist(int width, int height, std::vector<float> data, float sum, const wchar_t* title);
	int m_width, m_height;
	
public:
	Screen(
		const std::function<void(boost::gregorian::date, float)> &populate_Data,
		const std::vector<cl::sycl::device> &AllDevice,
		const std::function<void(std::string, int)>& populate_DeviceWorkloadPreference,
		const std::function<AlgorithmParameter& ()> &parameterReference,
		const std::function<void()> initialiseAlgorithm,
		const std::function<void()> &algorithmIterate,
		const std::function<float()> &algorithmCompletionPercent,
		const std::function<bool()> &algorithmCompleted,
		const std::function<AlgorithmResponse()> &algorithmResonse,
		int width, int height
	);
	void Render();
	unsigned char* intelBackgroundImageBuffer;
	void DrawBackgrounImage();
	void DrawStockGraph();
	void DrawAlgorithm();
};

