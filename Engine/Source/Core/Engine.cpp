
#include "Engine.h"
#include "RvelaLog.h"
#include <fstream>
#include <filesystem>

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

	while (!glfwWindowShouldClose(m_Window->GetWindow()))
	{
		Time::update();
		glfwPollEvents();
		EventManager::dispatchEvents([](Event& event) {
			if (event.GetEventType() == EventType::KeyPressed) {
				KeyPressedEvent& keyEvent = static_cast<KeyPressedEvent&>(event);

				if (keyEvent.GetKeycode() == KeyCode::S) {
					LOG_INFO << "S tuþuna basýldý!";
				}
			}
			});

		Render();

		if (Input::IsKeyPressed(KeyCode::A))
		{
			LOG_INFO << "A key is pressed";
		}


	}

	Shutdown();
	
}

void Engine::Render()
{
	m_Renderer->Render(m_Window->GetWindow());
}

void Engine::Shutdown()
{
	glfwTerminate();
	m_Renderer->Shutdown();
	delete m_Window;
}



