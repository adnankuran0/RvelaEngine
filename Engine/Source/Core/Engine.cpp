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

	m_Window.Init();
	m_ProjectManager.LoadProject("C:\\RvelaEngine\\TestProject\\TestProject.rproj");
	m_AssetRegistry.Scan(ProjectManager::GetProjectPath() / "Assets");
	AssetManager& assetManager = AssetManager::Get();
	assetManager.Init(m_AssetRegistry);

	AudioManager::Init();

	m_Renderer.Init(m_Window.GetGLFWWindow());
	m_SceneManager.Init();
	m_RenderLayer = new RenderLayer(this);
	PushLayer(m_RenderLayer);
	Selection = SelectionManager(m_RenderLayer);
	if (s_Instance == nullptr)
	{
		s_Instance = this;
	}
	else
	{
		LOG_WARN("Another instance of Engine already exists!");
	}

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

	LOG_DEBUG(Time::GetFPS());
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
