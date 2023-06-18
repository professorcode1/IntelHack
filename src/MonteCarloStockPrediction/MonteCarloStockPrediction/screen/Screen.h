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
#include <random>
#include <map>
#include <iostream>
#include <fstream>
#include <cpr/cpr.h>
#include <future>
#include <nlohmann/json.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
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
	uint32_t number_of_simulations_to_run = 10;
};
enum class ScreenState {
	First,
	Second,
	Third,
	Sixth,
	SixPointFifth,
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
	void LoadSixthScreen(const cpr::Response& response);
	void SixthScreenRender();
	void LoadSixPointFifth();
	void SixPointFifthRender();
	void LoadSeventhScreen();
	void SeventhScreenRender();


	void generateLargeStockPlot(
		const std::map<boost::posix_time::ptime, float>& dateMap,
		const std::string& stockName
	);

	std::function<void(boost::posix_time::ptime, float)> m_populate_Data;
	std::function<std::vector<std::vector<float> >(uint32_t, uint32_t) > m_predict;
	
	unsigned char* largeStocksPlot;
	unsigned char* largePredictedStocksPlot;
	ScatterPlotSettings* stocksSettings;
	std::map<boost::posix_time::ptime, float> internalStockData;
	std::future<void> prediction_async;

	unsigned char* algorithm_progress_screen;
	unsigned char* genereateHist(int width, int height, std::vector<float> data, float sum, const wchar_t* title);
	int m_width, m_height;

	void createThePredictionPlot(
		const std::vector<float>& StocksData,
		const std::vector<std::vector<float> >& prediction
	);
	void predict();
public:
	Screen(
		const std::function<void(boost::posix_time::ptime, float)> &populate_Data,
		const std::function<std::vector<std::vector<float> >(uint32_t, uint32_t) > &predict,
		int width, int height
	);
	void Render();
	unsigned char* intelBackgroundImageBuffer;
	unsigned char* pleaseWaitBurnInImageBuffer;
	void DrawBackgrounImage();
	void DrawStockGraph();
	void DrawAlgorithm();
	void DrawPrediction();
	void DrawBurnInPleaseWaitImage();
};

void LoadBitMapIntoOpenGLFormat(
	unsigned char* largeStocksPlot,
	uint32_t m_width, uint32_t m_height,
	RGBABitmapImageReference* canvasReference
);