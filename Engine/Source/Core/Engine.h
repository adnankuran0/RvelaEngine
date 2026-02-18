#pragma once
#include "Window.h"
#include "Rendering/Renderer.h"
#include "Scene/SceneManager.h"
#include "Assets/AssetRegistry.h"
#include "Utils/ProjectManager.h"
#include "Scene/SelectionManager.h"
#include "LayerStack.h"

namespace rv {

class EditorCamera;
class ICamera;

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
	Window& GetWindow() noexcept { return m_Window; }
	Scene& GetActiveScene() noexcept { return m_SceneManager.GetActiveScene(); }
	ProjectManager& GetProjectManager() noexcept { return m_ProjectManager; }
	SceneManager& GetSceneManager() noexcept { return m_SceneManager; }
	AssetRegistry& GetAssetRegistry() noexcept { return m_AssetRegistry; }

	CameraSystem& GetCameraSystem() noexcept { return m_SceneManager.GetActiveScene().GetCameraSystem(); }
	EditorCamera* GetEditorCamera() const noexcept { return m_EditorCamera; }
	Camera* GetSceneCamera() noexcept { return GetCameraSystem().GetActiveCamera(); }
	ICamera* GetCamera() noexcept;

	RenderLayer& GetRenderLayer() noexcept { return *m_RenderLayer; }

	inline void SetEditorCamera(EditorCamera* editorCam) { m_EditorCamera = editorCam; }

	SelectionManager Selection;
private:

	LayerStack m_LayerStack;

	RenderLayer* m_RenderLayer;

	void HandleEvents() noexcept;

	static Engine* s_Instance;
	EditorCamera* m_EditorCamera = nullptr;
	Window m_Window;
	Renderer m_Renderer;
	ProjectManager m_ProjectManager;
	SceneManager m_SceneManager;
	AssetRegistry m_AssetRegistry;

};

}