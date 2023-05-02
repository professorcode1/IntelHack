#pragma once
#include "../vendor/iamgui/imgui.h"
#include "../vendor/iamgui/imgui_impl_glfw.h"
#include "../vendor/iamgui/imgui_impl_opengl3.h"
#include <string>
#include <map>
#include <iostream>
struct FirstScreen {
	char NameToSymbolCSVFile[500];
	char APIKeyFile[500];
};
struct SecondScreen {
	std::map<std::string, std::string> CompanyNameToSymbol;
};
enum class ScreenState {
	First,
	Second
};
class Screen
{
private:
	FirstScreen firstScreen;
	ScreenState screenstate;

	void FirstScreenRender();
public:
	Screen();
	void Render();
};

