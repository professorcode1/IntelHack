#pragma once
#include "vendor/iamgui/imgui.h"
#include "vendor/iamgui/imgui_impl_glfw.h"
#include "vendor/iamgui/imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h> 
#include <iostream>
#include "screen/Screen.h"

class Application
{
private:
	GLFWwindow* window;
	Screen screen;
public:
	Application();
	~Application();
	void main_loop();
};

