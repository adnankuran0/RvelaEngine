
#include "Engine.h"
#include "RvelaLog.h"


Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
	RvelaLog::Init("log.txt");

	//TODO: try to make initialization of the window with stack allocation
	m_Window = new Window();
	if (s_Instance == nullptr)
	{
		s_Instance = this;
	}
	else
	{
		LOG_WARNING << "Another instance is already created!";
	}

	m_Renderer = new Renderer();
	m_Renderer->Init();
}

Engine::~Engine()
{		
	Shutdown();
}



void Engine::Run()
{

	while (!glfwWindowShouldClose(m_Window->GetGLFWWindow()))
	{
		Time::update();
		glfwPollEvents();
		EventManager::dispatchEvents([](Event& event) {
			if (event.GetEventType() == EventType::KeyPressed) {
				KeyPressedEvent& keyEvent = static_cast<KeyPressedEvent&>(event);

				if (keyEvent.GetKeycode() == KeyCode::S) {
					LOG_INFO << "S is pressed!";
				}
			}
			if (event.GetEventType() == EventType::KeyReleased) {
				KeyReleasedEvent& keyEvent = static_cast<KeyReleasedEvent&>(event);

				if (keyEvent.GetKeycode() == KeyCode::S) {
					LOG_INFO << "S is released!";
				}
			}
			if (event.GetEventType() == EventType::MouseButtonReleased)
			{
				MouseButtonPressedEvent& mouseEvent = static_cast<MouseButtonPressedEvent&>(event);

				if (mouseEvent.GetMouseCode() == MouseCode::Button0)
				{
					LOG_INFO << "Left Click released!";
				}
			}
			if (event.GetEventType() == EventType::MouseScrolled)
			{
				MouseScrolledEvent& mouseEvent = static_cast<MouseScrolledEvent&>(event);
				LOG_INFO << "Mouse scrolled " << mouseEvent.GetOffset();
			}
			if (event.GetEventType() == EventType::WindowResized)
			{
				WindowResizedEvent& windowEvent = static_cast<WindowResizedEvent&>(event);
				LOG_INFO << windowEvent.GetWidth() << " " << windowEvent.GetHeight();
			}

			});

		Render();

		if (Input::IsKeyPressed(KeyCode::A))
		{
			LOG_INFO << "A key is holding";
		}

		EventManager::clearEvents();

	}

	Shutdown();
	
}

void Engine::Render()
{
	m_Renderer->Render(m_Window->GetGLFWWindow());
}

void Engine::Shutdown()
{
	glfwTerminate();
	m_Renderer->Shutdown();
	delete m_Window;
}



