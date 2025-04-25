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

	m_SceneManager = std::make_unique<SceneManager>();
}

Engine::~Engine()
{		
	Shutdown();
}


void Engine::Run()
{
	glfwPollEvents();

	//std::cout << "Texture count: " << TextureManager::GetTextureCount() << " Material count: " << MaterialManager::GetMaterialCount() << std::endl;
	
	EventManager::DispatchEvents([this](Event& event) {
		// Handle mouse movement events
		if (event.GetEventType() == EventType::MouseMoved) {
			auto mouseEvent = dynamic_cast<MouseMovedEvent*>(&event);
			if (mouseEvent) {
				editorCamera.onMouseMoved(
					mouseEvent->GetX(),
					mouseEvent->GetY(),
					m_Window->GetGLFWWindow()
				);
			}
		}

		// Handle mouse scroll events when the left mouse button is pressed
		if (event.GetEventType() == EventType::MouseScrolled &&
			Input::IsMouseButtonPressed(MouseCode::Button1)) {
			auto* scrollEvent = dynamic_cast<MouseScrolledEvent*>(&event);
			if (scrollEvent) {
				editorCamera.SprintSpeed += scrollEvent->GetYOffset();
				editorCamera.SprintSpeed = std::clamp(editorCamera.SprintSpeed, 2.5f, 30.0f);
			}
		}
		});

	editorCamera.Update();

	Render();

	EventManager::ClearEvents();
	m_Scene->Update();
	
}

void Engine::Render()
{
	Renderer::StartFrame();

	std::vector<PointLightData> pointLights;

	auto pointLightView = m_Scene->GetRegistry().view<PointLightComponent, WorldTransformComponent>();
	for (auto entity : pointLightView) {
		auto& light = m_Scene->GetComponent<PointLightComponent>(entity);
		auto& transform = m_Scene->GetComponent<WorldTransformComponent>(entity);

		PointLightData data;
		data.position = glm::vec3(transform.GetMatrix()[3]); // mat4'ün son sütunu pozisyon
		data.color = light.color;
		data.intensity = light.intensity;
		data.radius = light.radius;

		pointLights.push_back(data);
	}

	DirectionalLightData mainDirLight;
	bool hasDirLight = false;

	auto dirLightView = m_Scene->GetRegistry().view<DirectionalLightComponent, WorldTransformComponent>();
	for (auto entity : dirLightView) {
		auto [light, transform] = dirLightView.get<DirectionalLightComponent, WorldTransformComponent>(entity);
		mainDirLight.direction = transform.GetForward();
		mainDirLight.color = light.color;
		mainDirLight.intensity = light.intensity;
		mainDirLight.castShadows = light.castShadows;
		hasDirLight = true;
		break;
	}

	auto RenderableView = m_Scene->GetRegistry().view<MeshRendererComponent, MaterialComponent>();

	Renderer::RenderSkybox(editorCamera);
	for (auto entity : RenderableView)
	{
		auto& mesh = m_Scene->GetComponent<MeshRendererComponent>(entity);
		auto& metarial = m_Scene->GetComponent<MaterialComponent>(entity);
		auto& transform = m_Scene->GetComponent<WorldTransformComponent>(entity);

		if (hasDirLight) {
			Renderer::Render(transform, mesh, metarial, editorCamera, pointLights, &mainDirLight);
		}
		else {
			Renderer::Render(transform, mesh, metarial, editorCamera, pointLights, nullptr);
		}

	}

	Renderer::EndFrame();
}


void Engine::Shutdown()
{
	glfwTerminate();
	m_Renderer->Shutdown();
}



