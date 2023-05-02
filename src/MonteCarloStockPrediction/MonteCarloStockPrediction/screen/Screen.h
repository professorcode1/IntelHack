#pragma once
#include "../vendor/iamgui/imgui.h"
#include "../vendor/iamgui/imgui_impl_glfw.h"
#include "../vendor/iamgui/imgui_impl_opengl3.h"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include "Utils.h"
struct FirstScreen {
	char NameToSymbolCSVFile[500];
	char APIKeyFile[500];
};
struct SecondScreen {
	std::map<std::string, std::string> CompanyNameToSymbol;
	std::string APIKey;
};
enum class ScreenState {
	First,
	Second
};
class Screen
{
private:
	FirstScreen firstScreen;
	SecondScreen secondScreen;
	ScreenState screenstate;

	void FirstScreenRender();
	void LoadSecondScreen();
	void SecondScreenRender();
public:
	Screen();
	void Render();
};

