
#include "Engine.h"
#include "Input/Input.h"
#include "RvelaLog.h"
#include <iostream>



Engine* Engine::s_Instance = nullptr;

Engine::Engine()
{
	//TODO: try to make initialization of the window with stack allocation
	m_Window = new Window();
	if (s_Instance == nullptr)
	{
		s_Instance = this;
		
	}
	else
	{
		std::cout << "Another instance is already created!" << std::endl;
	}

	RvelaLog::Init("log.txt");

}

Engine::~Engine()
{		
	Shutdown();
}



void Engine::Run()
{
	LOG_DEBUG << "Porno";

	
	while (!glfwWindowShouldClose(m_Window->GetWindow()))
	{
		Render();
		if (Input::IsKeyPressed(KeyCode::A))
		{
			std::cout << "A key is pressed" << std::endl;
		}

		/* Poll for and process events */
		glfwPollEvents();
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



