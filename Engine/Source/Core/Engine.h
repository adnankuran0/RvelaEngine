#pragma once
#define GLFW_INCLUDE_NONE
#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include "Window.h"
#include "Input/Input.h"
#include "Event/EventManager.h"
#include <iostream>

class Engine
{
public:
	Engine();
	~Engine();

	void Run();
	void Render();
	void Shutdown();
	

	static Engine* Get() { return s_Instance; }
	Window* GetWindow() const { return m_Window; }

private:
	static Engine* s_Instance;
	Window* m_Window;
};