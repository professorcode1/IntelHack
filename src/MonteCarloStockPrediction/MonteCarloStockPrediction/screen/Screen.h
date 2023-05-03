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
};
enum class ScreenState {
	First,
	Second,
	Third
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
public:
	Screen();
	void Render();
};

