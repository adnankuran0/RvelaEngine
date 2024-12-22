
#include "Engine.h"
#include "RvelaLog.h"


Engine* Engine::s_Instance = nullptr;
Scene scene;


static float vertices[] = {
	//   Position             Normal             UV
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,

		-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

		 -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
		  0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		  0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
		 -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,

		 -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
		  0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
		  0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
		 -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
};

static unsigned int indices[] = {
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	8, 9, 10, 8, 10, 11,
	12, 14, 13, 12, 15, 14,
	16, 17, 18, 16, 18, 19,
	20, 22, 21, 20, 23, 22
};

Camera editorCamera(glm::vec3(0.0f, 0.0f, 3.0f));

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
	m_Renderer->Init(m_Window->GetGLFWWindow());


	entt::entity entity = scene.createEntity("test");
	scene.addComponent<MeshComponent>(entity,  vertices, sizeof(vertices), indices, sizeof(indices) );
	scene.addComponent<MetarialComponent>(entity, "D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/vertex.glsl",
		"D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/fragment.glsl","D:/GitHub/RvelaEngine/Engine/Source/Resources/Textures/metal");

	entt::entity entity2 = scene.createEntity("test2");
	scene.addComponent<MeshComponent>(entity2, vertices, sizeof(vertices), indices, sizeof(indices));
	scene.addComponent<MetarialComponent>(entity2, "D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/vertex.glsl",
		"D:/GitHub/RvelaEngine/Engine/Source/Resources/Shaders/fragment.glsl", "D:/GitHub/RvelaEngine/Engine/Source/Resources/Textures/terrain");


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
		EventManager::dispatchEvents([this](Event& event) {
			if (event.GetEventType() == EventType::WindowResized)
			{
				WindowResizedEvent& windowEvent = static_cast<WindowResizedEvent&>(event);
				LOG_INFO << windowEvent.GetWidth() << " " << windowEvent.GetHeight();
			}
			if (event.GetEventType() == EventType::MouseMoved)
			{
				MouseMovedEvent& mouseEvent = static_cast<MouseMovedEvent&>(event);
				editorCamera.onMouseMoved(mouseEvent.GetX(), mouseEvent.GetY(),m_Window->GetGLFWWindow());
			}
			});

		auto& transform1 = scene.getComponent<TransformComponent>(entt::entity(0));
		transform1.rotation.x += 10.0 * Time::getDeltaTime();


		auto& transform2 = scene.getComponent<TransformComponent>(entt::entity(1));
		transform2.rotation.z += 10.0 * Time::getDeltaTime();

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

	Shutdown();
	
}

void Engine::Render()
{
	Renderer::StartFrame();
	auto view = scene.getRegistry().view<MeshComponent, MetarialComponent>();
	
	for (auto entity : view)
	{
		auto& mesh = scene.getComponent<MeshComponent>(entity);
		auto& metarial = scene.getComponent<MetarialComponent>(entity);
		auto& transform = scene.getComponent<TransformComponent>(entity);
		Renderer::Render(transform, mesh,metarial, editorCamera);
	}
	Renderer::EndFrame();
}

void Engine::Shutdown()
{
	glfwTerminate();
	m_Renderer->Shutdown();
	delete m_Window;
}



