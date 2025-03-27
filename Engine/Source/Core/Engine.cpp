#include "rvelapch.h"

#include "Engine.h"
#include "RvelaLog.h"
#include "Scene/Entity.h"


Engine* Engine::s_Instance = nullptr;





Camera editorCamera(glm::vec3(0.0f, 0.0f, 3.0f));

Engine::Engine()
{
	RvelaLog::Init("log.txt");

	//TODO: try to make initialization of the window with stack allocation
	m_Window = std::make_unique<Window>();
	if (s_Instance == nullptr)
	{
		s_Instance = this;
	}
	else
	{
		LOG_WARNING << "Another instance is already created!";
	}
	m_Renderer = std::make_unique<Renderer>();
	m_Renderer->Init(m_Window->GetGLFWWindow());

	m_Scene = std::make_unique<Scene>();


}

Engine::~Engine()
{		
	Shutdown();
}


void Engine::Run()
{
	glfwPollEvents();
	
	EventManager::dispatchEvents([this](Event& event) {

		if (event.GetEventType() == EventType::MouseMoved)
		{
			MouseMovedEvent& mouseEvent = static_cast<MouseMovedEvent&>(event);
			editorCamera.onMouseMoved(mouseEvent.GetX(), mouseEvent.GetY(),m_Window->GetGLFWWindow());
		}
	
		});

	editorCamera.Update();

	Render();

	EventManager::clearEvents();

	m_Scene->Update();
	
}

void Engine::Render()
{
	Renderer::StartFrame();

	
	auto RenderableView = m_Scene->GetRegistry().view<MeshComponent, MaterialComponent>();

	for (auto entity : RenderableView)
	{
		auto& mesh = m_Scene->GetComponent<MeshComponent>(entity);
		auto& metarial = m_Scene->GetComponent<MaterialComponent>(entity);
		auto& transform = m_Scene->GetComponent<WorldTransformComponent>(entity);

		Renderer::Render(transform, mesh, metarial, editorCamera);

	}
	

	
	Renderer::EndFrame();
}

void Engine::Shutdown()
{
	glfwTerminate();
	m_Renderer->Shutdown();
}



