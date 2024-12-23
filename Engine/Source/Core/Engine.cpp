
#include "Engine.h"
#include "RvelaLog.h"
#include "../Resources/cube.h"
#include "Scene/Entity.h"


Engine* Engine::s_Instance = nullptr;
Scene scene;

Entity entity = scene.CreateEntity("test");


Entity entity2 = scene.CreateEntity("test2");


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

	entity.AddComponent<MeshComponent>(vertices, sizeof(vertices), indices, sizeof(indices));
	entity.AddComponent<MetarialComponent>("D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/vertex.glsl",
		"D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/fragment.glsl", "D:/GitHub/RvelaEngine/Engine/Source/Resources/Textures/metal");

	entity2.AddComponent<MeshComponent>(vertices, sizeof(vertices), indices, sizeof(indices));
	entity2.AddComponent<MetarialComponent>("D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/vertex.glsl",
		"D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/fragment.glsl", "D:/GitHub/RvelaEngine/Engine/Source/Resources/Textures/terrain");


}

Engine::~Engine()
{		
	Shutdown();
}



void Engine::Run()
{

	Time::update();
	glfwPollEvents();
	EventManager::dispatchEvents([this](Event& event) {

		if (event.GetEventType() == EventType::MouseMoved)
		{
			MouseMovedEvent& mouseEvent = static_cast<MouseMovedEvent&>(event);
			editorCamera.onMouseMoved(mouseEvent.GetX(), mouseEvent.GetY(),m_Window->GetGLFWWindow());
		}
		});

	auto& transform1 = entity.GetComponent<TransformComponent>();
	transform1.rotation.x += 10.0f * Time::getDeltaTime();


	auto& transform2 = entity2.GetComponent<TransformComponent>();
	transform2.rotation.z += 10.0f * Time::getDeltaTime();


	if (Input::IsKeyPressed(KeyCode::Left))
	{
		transform2.position.x -= 1.0f * Time::getDeltaTime();
	}
	if (Input::IsKeyPressed(KeyCode::Right))
	{
		transform2.position.x += 1.0f * Time::getDeltaTime();
	}
	if (Input::IsKeyPressed(KeyCode::Down))
	{
		transform2.position.z += 1.0f * Time::getDeltaTime();
	}
	if (Input::IsKeyPressed(KeyCode::Up))
	{
		transform2.position.z -= 1.0f * Time::getDeltaTime();
	}
		

	editorCamera.Update();
		

	Render();

	EventManager::clearEvents();



	
}

void Engine::Render()
{
	Renderer::StartFrame();
	auto view = scene.GetRegistry().view<MeshComponent, MetarialComponent>();
	
	for (auto entity : view)
	{
		auto& mesh = scene.GetComponent<MeshComponent>(entity);
		auto& metarial = scene.GetComponent<MetarialComponent>(entity);
		auto& transform = scene.GetComponent<TransformComponent>(entity);
		Renderer::Render(transform, mesh,metarial, editorCamera);
	}
	Renderer::EndFrame();
}

void Engine::Shutdown()
{
	glfwTerminate();
	m_Renderer->Shutdown();
}



