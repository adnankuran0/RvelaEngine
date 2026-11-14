#include "rvelapch.h"
#include "Engine.h"
#include "Core/VFS.h"
#include "Core/Log.h"
#include "Renderer/RenderLayer.h"
#include "Input/Input.h"
#include "Event/EventManager.h"
#include "Time.h"
#include "Profiler.h"
#include <Renderer/EditorCamera.h>
#include <Event/MouseEvents.h>
#include "Renderer/DebugRenderer.h"
#include "Asset/AssetManager.h"
#include "Renderer/Texture.h"
#include "Audio/AudioManager.h"

using namespace rv;

Engine* Engine::s_Instance = nullptr;

Engine::Engine()
	: Engine(WindowData{})
{
}

Engine::Engine(const WindowData& windowData)
	: m_Window(windowData)
{
	if (s_Instance == nullptr)
		s_Instance = this;
	else
		LOG_WARN("Another instance of Engine already exists!");

	RvelaLog::Init("log.txt");

	InitResourcePaths();

	m_Window.Init();
	AssetManager& assetManager = AssetManager::Get();
	assetManager.Init(m_AssetRegistry);

	AudioManager::Init();

	m_Renderer.Init(m_Window.GetGLFWWindow());

	m_SceneManager.Init();
	m_RenderLayer = new RenderLayer(this);
	PushLayer(m_RenderLayer);
	Selection = SelectionManager(m_RenderLayer);
}

bool Engine::LoadProject(const std::string& projectFilePath)
{
	if (!m_ProjectManager.LoadProject(projectFilePath))
	{
		LOG_ERROR("Failed to load project: {}", projectFilePath);
		return false;
	}

	AudioManager::Get().StopAllInstances();

	AssetManager::Get().UnloadAll();
	if (VFS::Get().IsMounted())
	{
		m_AssetRegistry.ScanFromVFS();
	}
	else
	{
		m_AssetRegistry.Scan(ProjectManager::GetAssetDirectory());
	}

	return true;
}

bool Engine::LoadStartScene()
{
	auto activeProj = m_ProjectManager.GetActiveProject();
	if (!activeProj)
		return false;

	const auto& settings = activeProj->GetSettings();
	std::string activeSceneName = "EmptyScene";

	if (!settings.startScene.empty())
	{
		bool sceneFound = false;
		std::filesystem::path scenePath;
		if (VFS::Get().IsMounted())
		{
			if (VFS::Get().FileExists(settings.startScene))
			{
				scenePath = settings.startScene;
				sceneFound = true;
			}
			else if (VFS::Get().FileExists("Assets/" + settings.startScene))
			{
				scenePath = "Assets/" + settings.startScene;
				sceneFound = true;
			}
		}
		if (!sceneFound)
		{
			scenePath = ProjectManager::GetProjectPath() / settings.startScene;
			sceneFound = std::filesystem::exists(scenePath);
		}

		if (sceneFound)
		{
			m_SceneManager.LoadScene(scenePath.string());
			m_SceneManager.Update(); // Immediately apply pending scene load
			activeSceneName = scenePath.stem().string();
		}
		else
		{
			auto emptyScene = m_SceneManager.CreateScene("EmptyScene");
			Path defaultHdr = ENGINE_PATH("Textures/skybox/environment.hdr");
			if (defaultHdr.Exists())
				emptyScene->GetEnvironment().GetSkybox().InitHDR(defaultHdr);
			m_SceneManager.SetActiveScene(std::move(emptyScene));
		}
	}
	else
	{
		auto emptyScene = m_SceneManager.CreateScene("EmptyScene");
		Path defaultHdr = ENGINE_PATH("Textures/skybox/environment.hdr");
		if (defaultHdr.Exists())
			emptyScene->GetEnvironment().GetSkybox().InitHDR(defaultHdr);
		m_SceneManager.SetActiveScene(std::move(emptyScene));
	}

	std::string title = "Rvela Engine - [" + activeProj->name + "] - " + activeSceneName;
	m_Window.SetTitle(title);

	m_Window.SetVSync(settings.vsync);
	Time::SetMaxFPS(settings.maxFPS);

	return true;
}

bool Engine::OpenProject(const std::string& projectFilePath)
{
	if (!LoadProject(projectFilePath))
		return false;

	return LoadStartScene();
}

Engine::~Engine()
{
	Shutdown();
	if (s_Instance == this)
		s_Instance = nullptr;
}

void Engine::PushLayer(Layer* layer)
{
	m_LayerStack.PushLayer(layer);
}

void Engine::PopLayer(Layer* layer)
{
	m_LayerStack.PopLayer(layer);
}

void Engine::Update()
{
	for (Layer* layer : m_LayerStack)
	{
		layer->OnUpdate();
	}

	m_SceneManager.Update();
	AudioManager::Get().Update(GetCamera());
}

void Engine::FixedUpdate()
{
	m_SceneManager.FixedUpdate();

	for (Layer* layer : m_LayerStack)
		layer->OnFixedUpdate();
}

void Engine::LateUpdate()
{
	m_SceneManager.LateUpdate();

	for (Layer* layer : m_LayerStack)
		layer->OnLateUpdate();
}

void Engine::Run()
{
	LOG_INFO("Engine has started!");

	m_IsRunning = true;
	while (m_IsRunning && !glfwWindowShouldClose(GetWindow().GetGLFWWindow()))
	{
		Profiler::Get().BeginFrame();

		{
			RV_PROFILE_CPU("Poll Events");
			glfwPollEvents();
			Input::BeginFrame();
			HandleEvents();
		}

		Time::Update();

		{
			RV_PROFILE_CPU("Update");
			DebugRenderer::Get().BeginFrame();
			Update();
		}

		{
			RV_PROFILE_CPU("Fixed Update");
			while (Time::ShouldRunFixedUpdate())
			{
				FixedUpdate();
				Time::ConsumeFixedDeltaTime();
			}
		}

		{
			RV_PROFILE_CPU("Late Update");
			LateUpdate();
		}

		{
			RV_PROFILE_CPU("Render");
			Render();
		}

		{
			RV_PROFILE_CPU("Input & Event Clear");
			EventManager::ClearEvents();
			Input::Update();
		}

		Profiler::Get().EndFrame();
	}
	LOG_INFO("Engine has stopped!");
}

void Engine::HandleEvents() noexcept
{
	EventManager::DispatchEvents([this](Event& event) 
		{
		
			auto layer = m_LayerStack.end();
			while (layer != m_LayerStack.begin() && !event.Handled)
			{
				--layer;
				(*layer)->OnEvent(event);
			}
		});
}

Camera* Engine::GetCamera() noexcept
{
	Scene& scene = m_SceneManager.GetActiveScene();
	Camera* sceneCamera = scene.GetCameraSystem().GetActiveCamera();

	bool useEditorCamera = scene.GetState() == SceneState::EDIT || !sceneCamera;

	return useEditorCamera ? m_EditorCamera : sceneCamera;
}

void Engine::Render()
{
	m_Renderer.StartFrame();

	for (Layer* layer : m_LayerStack)
		layer->OnRender();
	


	m_Renderer.EndFrame();
	
	{
		RV_PROFILE_CPU("Swap Buffers");
		m_Window.SwapBuffers();
	}
	
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		LOG_ERROR("OpenGL error: {}", err);
	}
}


void Engine::Shutdown()
{
	m_Window.Shutdown();
	m_Renderer.Shutdown();

	glfwTerminate();
}

void Engine::Quit() noexcept
{
	m_IsRunning = false;
	m_Window.Close();
}

void Engine::InitResourcePaths()
{
	auto exeDir = Path::GetExecutableDirectory();
	auto curDir = std::filesystem::current_path();

	std::vector<std::filesystem::path> engineCandidates = {
		exeDir / "Resources" / "Engine",
		curDir / "Resources" / "Engine",
		exeDir.parent_path() / "Resources" / "Engine",
		exeDir.parent_path().parent_path() / "Resources" / "Engine",
		exeDir.parent_path().parent_path().parent_path() / "Resources" / "Engine"
	};

#if defined(RVELA_ROOT_DIR)
	engineCandidates.push_back(std::filesystem::path(RVELA_ROOT_DIR) / "Resources" / "Engine");
#endif

	std::filesystem::path engineRes;
	for (const auto& path : engineCandidates)
	{
		std::error_code ec;
		if (std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec))
		{
			engineRes = std::filesystem::canonical(path, ec);
			if (ec) engineRes = path;
			break;
		}
	}

	if (engineRes.empty())
	{
		engineRes = exeDir / "Resources" / "Engine";
		if (!VFS::Get().IsMounted())
		{
			LOG_WARN("Could not locate existing Resources/Engine directory. Falling back to: {}", engineRes.string());
		}
	}
	else
	{
		LOG_INFO("Engine Resources located at: {}", engineRes.string());
	}
	Path::SetEngineResourcesPath(engineRes);

	std::vector<std::filesystem::path> editorCandidates = {
		exeDir / "Resources" / "Editor",
		curDir / "Resources" / "Editor",
		exeDir.parent_path() / "Resources" / "Editor",
		exeDir.parent_path().parent_path() / "Resources" / "Editor",
		exeDir.parent_path().parent_path().parent_path() / "Resources" / "Editor"
	};

#if defined(RVELA_ROOT_DIR)
	editorCandidates.push_back(std::filesystem::path(RVELA_ROOT_DIR) / "Resources" / "Editor");
#endif

	std::filesystem::path editorRes;
	for (const auto& path : editorCandidates)
	{
		std::error_code ec;
		if (std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec))
		{
			editorRes = std::filesystem::canonical(path, ec);
			if (ec) editorRes = path;
			break;
		}
	}

	if (editorRes.empty())
	{
		editorRes = exeDir / "Resources" / "Editor";
	}
	Path::SetEditorResourcesPath(editorRes);
}
