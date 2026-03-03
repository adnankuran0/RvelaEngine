#include "rvelapch.h"
#include "Engine.h"
#include "Core/Log.h"
#include "Rendering/RenderLayer.h"
#include "Input/Input.h"
#include "Event/EventManager.h"
#include "Time.h"
#include <Rendering/EditorCamera.h>
#include <Event/MouseEvents.h>

using namespace rv;

Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
	RvelaLog::Init("log.txt");

	m_Window.Init();
	m_ProjectManager.LoadProject("C:\\RvelaEngine\\TestProject\\TestProject.rproj");
	m_AssetRegistry.Init(m_ProjectManager.GetProjectPath()); //TODO: Make this works with assets path
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
	EventManager::DispatchEvents([this](Event& event) {
		switch (event.GetEventType())
		{
		case EventType::MouseMoved:
		{
			if (auto* mouseEvent = dynamic_cast<MouseMovedEvent*>(&event) )
			{
				if (GetActiveScene().GetState() == SceneState::EDIT && m_EditorCamera)
				{
					m_EditorCamera->OnMouseMoved(
						mouseEvent->GetX(),
						mouseEvent->GetY(),
						m_Window.GetGLFWWindow()
					);
				}
				
			}
			break;
		}
		case EventType::MouseScrolled:
		{
			if (Input::IsMouseButtonPressed(MouseCode::Button1))
			{
				if (auto* scrollEvent = dynamic_cast<MouseScrolledEvent*>(&event))
				{
					// Adjust sprint speed based on scroll input
					m_EditorCamera->MovementSpeed += scrollEvent->GetYOffset();
					m_EditorCamera->MovementSpeed = std::clamp(m_EditorCamera->MovementSpeed, 1.0f, 100.0f);
				}
			}
			break;
		}
		case EventType::KeyPressed:
		{
			if (Input::IsKeyJustPressed(KeyCode::End))
			{
				LOG_DEBUG("Shaders reloaded.");
				ShaderManager::ReloadAll();
			}
			break;
		}
		default:
			break;
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
		LOG_ERROR("OpenGL hatası: {}", err);
	}
}


void Engine::Shutdown()
{
	m_Window.Shutdown();
	m_Renderer.Shutdown();

	glfwTerminate();
}
