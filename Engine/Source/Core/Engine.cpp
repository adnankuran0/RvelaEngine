
#include "Engine.h"
#include "Input/Input.h"
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
}

Engine::~Engine()
{		
	Shutdown();
}



void Engine::Run()
{
	while (!glfwWindowShouldClose(m_Window->GetWindow()))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		/* Swap front and back buffers */
		glfwSwapBuffers(m_Window->GetWindow());

		if (Input::IsKeyPressed(KeyCode::A))
		{
			std::cout << "A key is pressed" << std::endl;
		}

		/* Poll for and process events */
		glfwPollEvents();
	}

	
	glfwTerminate();
}

void Engine::Render()
{

}

void Engine::Shutdown()
{
	delete m_Window;
}



