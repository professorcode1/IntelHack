#pragma once
#include "Engine/Algorithm.h"
#include "vendor/iamgui/imgui.h"
#include "vendor/iamgui/imgui_impl_glfw.h"
#include "vendor/iamgui/imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h> 
#include <iostream>
#include <fstream>
#include "screen/Screen.h"
#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stbi/stbi.h"
#include "Engine/SYCLComputer.h"


class Application
{
private:
	GLFWwindow* window;
	Screen screen;
	std::map<boost::gregorian::date, float> data;
	std::map<std::string, int> deviceNameToWorkload;
	AlgorithmParameter parameter;
	WigginsAlgorithm* HMC_Wiggins;
	SYCLComputer SyclComputer;

public:
	Application();
	~Application();
	void main_loop();
};

