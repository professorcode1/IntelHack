#pragma once
#include "../vendor/iamgui/imgui.h"
#include "../vendor/iamgui/imgui_impl_glfw.h"
#include "../vendor/iamgui/imgui_impl_opengl3.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include "Utils.h"
#include <cpr/cpr.h>
#include <future>
struct FirstScreen {
	char NameToSymbolCSVFile[500];
	char APIKeyFile[500];
};
struct SecondScreen {
	std::map<std::string, std::string> CompanyNameToSymbol;
	std::string APIKey;
};
struct ThirdScreen {
	std::string StockName;
	std::string StockSymbol;
	cpr::AsyncResponse *APICallResponse;
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
		
	ScreenState screenstate;
	void FirstScreenRender();
	void LoadSecondScreen();
	void SecondScreenRender();
	void LoadThirdScreen(std::map<std::string, std::string>::iterator StockSelected);
	void ThirdScreenRender();
public:
	Screen();
	void Render();
};

