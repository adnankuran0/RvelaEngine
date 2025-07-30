#include "rvelapch.h"
#include "Engine.h"
#include "Core/Log.h"
#include "Renderer/RenderLayer.h"

Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
	RvelaLog::Init("log.txt");

	m_Window.Init();
	m_ProjectManager.LoadProject("C:\\RvelaEngine\\TestProject\\TestProject.rproj");
	m_Renderer.Init(m_Window.GetGLFWWindow());
	m_AssetRegistry.Init(m_ProjectManager.GetProjectPath()); //TODO: Make this works with assets path
	m_Scene = std::make_unique<Scene>();
	PushLayer(new RenderLayer(this));

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
	LOG_INFO("Engine has started!");
	while (!glfwWindowShouldClose(GetWindow().GetGLFWWindow()))
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
			if (auto* mouseEvent = dynamic_cast<MouseMovedEvent*>(&event))
			{
				editorCamera->onMouseMoved(
					mouseEvent->GetX(),
					mouseEvent->GetY(),
					m_Window.GetGLFWWindow()
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
	m_Renderer.StartFrame();

	for (Layer* layer : m_LayerStack)
		layer->OnRender();

	m_Renderer.EndFrame();

	glfwSwapBuffers(GetWindow().GetGLFWWindow());
}


void Engine::Shutdown()
{
	m_Window.Shutdown();
	m_Renderer.Shutdown();

	glfwTerminate();
}
