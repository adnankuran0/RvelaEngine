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
#include "Rendering/Renderer.h"
#include "Assets/AssetRegistry.h"
#include <Scene/CameraManager.h>

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
	inline Window& GetWindow() noexcept { return m_Window; }
	inline Scene& GetActiveScene() noexcept { return m_SceneManager.GetActiveScene(); }
	inline ProjectManager& GetProjectManager() noexcept { return m_ProjectManager; }
	inline SceneManager& GetSceneManager() noexcept { return m_SceneManager; }
	inline AssetRegistry& GetAssetRegistry() noexcept { return m_AssetRegistry; }
	inline CameraManager& GetCameraManager() noexcept { return m_CameraManager; }

	inline EditorCamera* GetCamera() const noexcept { return editorCamera; }
	inline void SetEditorCamera(EditorCamera* editorCam) { editorCamera = editorCam; }

	[[nodiscard]] inline void SetFinalTexture(GLuint textureID) { finalTexture = textureID; } //TODO: what the fuck are these doing here?
	[[nodiscard]] inline GLuint GetFinalTexture() { return finalTexture; }

private:
	GLuint finalTexture;

	LayerStack m_LayerStack;

	void HandleEvents() noexcept;
	

	static Engine* s_Instance;
	CameraManager m_CameraManager;
	EditorCamera* editorCamera = nullptr;
	Window m_Window;
	Renderer m_Renderer;
	ProjectManager m_ProjectManager;
	SceneManager m_SceneManager;
	AssetRegistry m_AssetRegistry;

};