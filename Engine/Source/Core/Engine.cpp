
#include "Engine.h"
#include "Input/Input.h"
#include "RvelaLog.h"
#include "Event/EventManager.h"
#include <iostream>



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


}

Engine::~Engine()
{		
	Shutdown();
}



void Engine::Run()
{
	while (!glfwWindowShouldClose(m_Window->GetWindow()))
	{
		glfwPollEvents();
		EventManager::dispatchEvents([](Event& event) {
			if (event.GetEventType() == EventType::KeyPressed) {
				KeyPressedEvent& keyEvent = static_cast<KeyPressedEvent&>(event);

				// 'E' tuþuna basýldýðýnda çýktý vereceðiz
				if (keyEvent.GetKeycode() == KeyCode::E) {
					LOG_INFO << "E tuþuna basýldý!";
				}
			}
			});


		Render();
		if (Input::IsKeyPressed(KeyCode::A))
		{
			std::cout << "A key is pressed" << std::endl;
		}

		EventManager::clearEvents();
	}

	Shutdown();
	
}

void Engine::Render()
{
	glClearColor(0.05, 0.0, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	/* Swap front and back buffers */
	glfwSwapBuffers(m_Window->GetWindow());

}

void Engine::Shutdown()
{
	glfwTerminate();
	delete m_Window;
}



