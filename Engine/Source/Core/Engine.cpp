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
{
	RvelaLog::Init("log.txt");

	Path::SetEngineResourcesPath(std::filesystem::path(RVELA_ROOT_DIR) / "Resources" / "Engine");
	Path::SetEditorResourcesPath(std::filesystem::path(RVELA_ROOT_DIR) / "Resources" / "Editor");

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

		glfwSwapInterval(settings.vsync ? 1 : 0);
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

	while (!glfwWindowShouldClose(GetWindow().GetGLFWWindow()))
	{
		glfwPollEvents();
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
		
			for (auto* layer : m_LayerStack)
			{
				layer->OnEvent(event);
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
