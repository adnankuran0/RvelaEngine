
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
			if (event.GetEventType() == EventType::WindowResized)
			{
				WindowResizedEvent& windowEvent = static_cast<WindowResizedEvent&>(event);
				LOG_INFO << windowEvent.GetWidth() << " " << windowEvent.GetHeight();
			}
			});

		Render();




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



