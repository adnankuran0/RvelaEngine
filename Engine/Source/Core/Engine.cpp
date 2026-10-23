#include "rvelapch.h"
#include "Engine.h"
#include "Core/Log.h"
#include "Renderer/RenderLayer.h"
#include "Input/Input.h"
#include "Event/EventManager.h"
#include "Time.h"
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
	RvelaLog::Init("log.txt");

	InitResourcePaths();


	m_Window.Init();
	AssetManager& assetManager = AssetManager::Get();
	assetManager.Init(m_AssetRegistry);

	AudioManager::Init();

	m_Renderer.Init(m_Window.GetGLFWWindow());
	if (s_Instance == nullptr)
		s_Instance = this;
	else
		LOG_WARN("Another instance of Engine already exists!");

	m_SceneManager.Init();
	m_RenderLayer = new RenderLayer(this);
	PushLayer(m_RenderLayer);
	Selection = SelectionManager(m_RenderLayer);
}

bool Engine::OpenProject(const std::string& projectFilePath)
{
	if (!m_ProjectManager.LoadProject(projectFilePath))
	{
		LOG_ERROR("Failed to load project: {}", projectFilePath);
		return false;
	}

	AudioManager::Get().StopAllInstances();

	AssetManager::Get().UnloadAll();
	m_AssetRegistry.Scan(ProjectManager::GetAssetDirectory());

	auto activeProj = m_ProjectManager.GetActiveProject();
	if (activeProj)
	{
		const auto& settings = activeProj->GetSettings();
		std::string activeSceneName = "EmptyScene";

		if (!settings.startScene.empty())
		{
			std::filesystem::path scenePath = ProjectManager::GetProjectPath() / settings.startScene;
			if (std::filesystem::exists(scenePath))
			{
				m_SceneManager.LoadScene(scenePath.string());
				m_SceneManager.Update(); // Immediately apply pending scene load
				activeSceneName = scenePath.stem().string();
			}
			else
			{
				m_SceneManager.SetActiveScene(m_SceneManager.CreateScene("EmptyScene"));
			}
		}
		else
		{
			m_SceneManager.SetActiveScene(m_SceneManager.CreateScene("EmptyScene"));
		}

		std::string title = "Rvela Engine - [" + activeProj->name + "] - " + activeSceneName;
		m_Window.SetTitle(title);

		m_Window.SetVSync(settings.vsync);
		Time::SetMaxFPS(settings.maxFPS);
	}

	return true;
}

Engine::~Engine()
{
	Shutdown();
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
	m_SceneManager.Update();
	AudioManager::Get().Update(GetCamera());

	for (Layer* layer : m_LayerStack)
	{
		layer->OnUpdate();
	}
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
		glfwPollEvents();
		Input::BeginFrame();
		HandleEvents();

		Time::Update();

		DebugRenderer::Get().BeginFrame();
		
		Update();
		while (Time::ShouldRunFixedUpdate())
		{
			FixedUpdate();
			Time::ConsumeFixedDeltaTime();
		}
		
		LateUpdate();


		Render();

		EventManager::ClearEvents();
		Input::Update();
		
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
	
	glfwSwapBuffers(GetWindow().GetGLFWWindow());
	
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
	if (m_Window.GetGLFWWindow())
	{
		glfwSetWindowShouldClose(m_Window.GetGLFWWindow(), GLFW_TRUE);
	}
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
		LOG_WARN("Could not locate existing Resources/Engine directory. Falling back to: {}", engineRes.string());
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
