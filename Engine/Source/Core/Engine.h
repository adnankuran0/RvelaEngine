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
#include "../Scene/SceneManager.h"
#include "Core/Utils/ProjectManager.h"

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
	Scene* GetScene() const { return m_Scene.get(); }
	ProjectManager* GetProjectManager() const { return m_ProjectManager.get(); }
	SceneManager* GetSceneManager() const { return m_SceneManager.get(); }
private:
	static Engine* s_Instance;
	std::unique_ptr<Window> m_Window;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<Scene> m_Scene;
	std::unique_ptr<ProjectManager> m_ProjectManager;
	std::unique_ptr<SceneManager> m_SceneManager;

};