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
	std::map<boost::gregorian::date, float> data;
};
enum class ScreenState {
	First,
	Second,
	Third,
	Fourth
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
public:
	Screen();
	void Render();
};

