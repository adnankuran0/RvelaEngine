#pragma once
#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include "Window.h"
#include "Input/Input.h"
#include "Event/EventManager.h"
#include "../Renderer/Renderer.h"
#include "Time.h"
#include <iostream>
#include "../Scene/Scene.h"
#include "../Scene/Components.h"



class Engine
{
public:
	Engine();
	~Engine();

	void Run();
	void Render();
	void Shutdown();
	

	static Engine* Get() { return s_Instance; }
	Window* GetWindow() const { return m_Window.get(); }



private:
	static Engine* s_Instance;
	std::unique_ptr<Window> m_Window;
	std::unique_ptr<Renderer> m_Renderer;

};