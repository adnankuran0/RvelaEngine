#pragma once
#include "Window.h"
#include "Input/Input.h"
#include "Event/EventManager.h"
#include "Time.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Scene/SceneManager.h"
#include "Scene/Entity.h"
#include "Utils/ProjectManager.h"
#include "LayerStack.h"
#include "Renderer/Renderer.h"

class Engine
{
public:
	Engine();
	~Engine();

	void Run();
	void FixedUpdate();
	void Update();
	void LateUpdate();
	void Render();
	void Shutdown();

	void PushLayer(Layer* layer);
	void PopLayer(Layer* layer);

	inline static Engine* Get() noexcept { return s_Instance; } 
	inline Window* GetWindow() const noexcept { return m_Window.get(); }
	inline Scene* GetScene() const noexcept { return m_Scene.get(); }
	inline ProjectManager* GetProjectManager() const noexcept { return m_ProjectManager.get(); }
	inline SceneManager* GetSceneManager() const noexcept { return m_SceneManager.get(); }

	inline EditorCamera* GetCamera() const noexcept { return editorCamera; }
	inline void SetEditorCamera(EditorCamera* editorCam) { editorCamera = editorCam; }
private:
	LayerStack m_LayerStack;

	void HandleEvents() noexcept;
	

	static Engine* s_Instance;

	EditorCamera* editorCamera;
	std::unique_ptr<Window> m_Window;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<Scene> m_Scene;
	std::unique_ptr<ProjectManager> m_ProjectManager;
	std::unique_ptr<SceneManager> m_SceneManager;

};