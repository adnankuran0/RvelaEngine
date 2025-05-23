#include "rvelapch.h"

#include "RvelaLog.h"
#include "Engine.h"
#include "Scene/Entity.h"
#include "Renderer/RenderLayer.h"
#include "Renderer/Passes/MeshPass.h"

Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
	RvelaLog::Init("log.txt");

	m_Window = std::make_unique<Window>();
	m_Renderer = std::make_unique<Renderer>();
	m_Scene = std::make_unique<Scene>();
	m_ProjectManager = std::make_unique<ProjectManager>();
	m_ProjectManager->LoadProject("D:\\Github\\RvelaEngine\\TestProject\\TestProject.rproj");
	m_SceneManager = std::make_unique<SceneManager>();
	editorCamera = nullptr;

	PushLayer(new RenderLayer(this));

	if (s_Instance == nullptr)
	{
		s_Instance = this;
	}
	else
	{
		LOG_WARNING << "Another instance of Engine already exists!";
	}

	m_Renderer->Init(m_Window->GetGLFWWindow());

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
	for (Layer* layer : m_LayerStack)
	{
		layer->OnUpdate();
	}

	m_Scene->Update();
}

void Engine::FixedUpdate()
{
	static double lastTime = glfwGetTime();
	double currentTime = glfwGetTime();
	lastTime = currentTime;
	for (Layer* layer : m_LayerStack)
		layer->OnFixedUpdate();
}

void Engine::LateUpdate()
{
	for (Layer* layer : m_LayerStack)
		layer->OnLateUpdate();
}

void Engine::Run()
{
	while (!glfwWindowShouldClose(GetWindow()->GetGLFWWindow()))
	{
		glfwPollEvents();
		HandleEvents();

		Time::Update();

		while (Time::ShouldRunFixedUpdate())
		{
			FixedUpdate();
			Time::ConsumeFixedDeltaTime();
		}

		Update();
		LateUpdate();

		Render();

		EventManager::ClearEvents();

		glfwSwapBuffers(GetWindow()->GetGLFWWindow());
	}
}

void Engine::HandleEvents() noexcept
{
	EventManager::DispatchEvents([this](Event& event) {
		switch (event.GetEventType())
		{
		case EventType::MouseMoved:
		{
			if (auto* mouseEvent = dynamic_cast<MouseMovedEvent*>(&event))
			{
				editorCamera->onMouseMoved(
					mouseEvent->GetX(),
					mouseEvent->GetY(),
					m_Window->GetGLFWWindow()
				);
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
					editorCamera->SprintSpeed += scrollEvent->GetYOffset();
					editorCamera->SprintSpeed = std::clamp(editorCamera->SprintSpeed, 2.5f, 30.0f);
				}
			}
			break;
		}
		default:
			break;
		}
		});
}

void Engine::Render()
{
	Renderer::StartFrame();

	for (Layer* layer : m_LayerStack)
		layer->OnRender();

	Renderer::EndFrame();
}


void Engine::Shutdown()
{
	if (m_Renderer)
		m_Renderer->Shutdown();

	glfwTerminate();
}
