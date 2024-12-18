
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



