#include "rvelapch.h"

#include "Engine.h"
#include "RvelaLog.h"
#include "Scene/Entity.h"

#include "Core/Utils/MaterialManager.h"
#include "Core/Utils/TextureManager.h"

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

	m_ProjectManager = std::make_unique<ProjectManager>();
}

Engine::~Engine()
{		
	Shutdown();
}


void Engine::Run()
{
	glfwPollEvents();

	//std::cout << "Texture count: " << TextureManager::GetTextureCount() << " Material count: " << MaterialManager::GetMaterialCount() << std::endl;
	
	EventManager::dispatchEvents([this](Event& event) {

		if (event.GetEventType() == EventType::MouseMoved)
		{
			MouseMovedEvent& mouseEvent = static_cast<MouseMovedEvent&>(event);
			editorCamera.onMouseMoved(mouseEvent.GetX(), mouseEvent.GetY(),m_Window->GetGLFWWindow());
		}

		if (event.GetEventType() == EventType::MouseScrolled && Input::IsMouseButtonPressed(MouseCode::Button1))
		{
			MouseScrolledEvent& scrollEvent = static_cast<MouseScrolledEvent&>(event);
			editorCamera.SprintSpeed += scrollEvent.GetYOffset() ;
			editorCamera.SprintSpeed = std::clamp(editorCamera.SprintSpeed, 2.5f, 30.0f);
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



