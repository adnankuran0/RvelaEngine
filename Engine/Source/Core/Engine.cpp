#include "rvelapch.h"

#include "RvelaLog.h"
#include "Engine.h"
#include "Scene/Entity.h"

Engine* Engine::s_Instance = nullptr;


/**
 * @brief Constructs the Engine, initializing core systems and singleton instance.
 * Initializes various subsystems like the window, renderer, scene, etc.
 */
Engine::Engine()
{
	RvelaLog::Init("log.txt");

	m_Window = std::make_unique<Window>();
	m_Renderer = std::make_unique<Renderer>();
	m_Scene = std::make_unique<Scene>();
	m_ProjectManager = std::make_unique<ProjectManager>();
	m_SceneManager = std::make_unique<SceneManager>();
	editorCamera = nullptr;

	if (s_Instance == nullptr)
	{
		s_Instance = this;
	}
	else
	{
		LOG_WARNING << "Another instance of Engine already exists!";
	}

	m_Renderer->Init(m_Window->GetGLFWWindow());

	Time::SetMaxFPS(0);
}

/**
 * @brief Destroys the Engine and shuts down all initialized systems.
 * Cleans up all resources before application termination.
 */
Engine::~Engine()
{
	Shutdown();
}

/**
 * @brief Adds a new layer to the engine's layer stack.
 * Layers are used to manage various systems like rendering, UI, etc.
 * @param layer A pointer to the layer to be added.
 */
void Engine::PushLayer(Layer* layer)
{
	m_LayerStack.PushLayer(layer);
}

/**
 * @brief Removes a layer from the engine's layer stack.
 * @param layer A pointer to the layer to be removed.
 */
void Engine::PopLayer(Layer* layer)
{
	m_LayerStack.PopLayer(layer);
}

/**
 * @brief Updates all layers and core systems of the engine.
 * This is where most of the game logic and system updates occur.
 */
void Engine::Update()
{
	for (Layer* layer : m_LayerStack)
	{
		layer->OnUpdate();
	}

	m_Scene->Update();
}

/**
 * @brief Performs fixed time updates for physics or other time-dependent systems.
 */
void Engine::FixedUpdate()
{
	static double lastTime = glfwGetTime();
	double currentTime = glfwGetTime();
	lastTime = currentTime;
	for (Layer* layer : m_LayerStack)
		layer->OnFixedUpdate();
}

/**
 * @brief Performs late updates for things that should occur after the main update.
 */
void Engine::LateUpdate()
{
	for (Layer* layer : m_LayerStack)
		layer->OnLateUpdate();
}

/**
 * @brief Main execution loop for the engine.
 * Handles events, updates, rendering, and the main game loop.
 */
void Engine::Run()
{
	while (!glfwWindowShouldClose(GetWindow()->GetGLFWWindow()))
	{
		glfwPollEvents();
		HandleEvents();

		Time::Update();

		while (Time::ShouldRunFixedUpdate())
		{
			FixedUpdate();
			Time::ConsumeFixedDeltaTime();
		}

		Update();
		LateUpdate();

		Render();

		EventManager::ClearEvents();

		glfwSwapBuffers(GetWindow()->GetGLFWWindow());
	}
}

/**
 * @brief Handles all queued events such as mouse movement, scroll, etc.
 * Dispatches events to the appropriate handlers.
 */
void Engine::HandleEvents() noexcept
{
	EventManager::DispatchEvents([this](Event& event) {
		switch (event.GetEventType())
		{
		case EventType::MouseMoved:
		{
			if (auto* mouseEvent = dynamic_cast<MouseMovedEvent*>(&event))
			{
				editorCamera->onMouseMoved(
					mouseEvent->GetX(),
					mouseEvent->GetY(),
					m_Window->GetGLFWWindow()
				);
			}
			break;
		}
		case EventType::MouseScrolled:
		{
			if (Input::IsMouseButtonPressed(MouseCode::Button1))
			{
				if (auto* scrollEvent = dynamic_cast<MouseScrolledEvent*>(&event))
				{
					// Adjust sprint speed based on scroll input
					editorCamera->SprintSpeed += scrollEvent->GetYOffset();
					editorCamera->SprintSpeed = std::clamp(editorCamera->SprintSpeed, 2.5f, 30.0f);
				}
			}
			break;
		}
		default:
			break;
		}
		});
}

/**
 * @brief Executes the rendering pipeline for the current frame.
 * Renders the scene, including skybox, mesh renderers, lights, etc.
 */
void Engine::Render()
{
	Renderer::StartFrame();

	std::vector<PointLightData> pointLights = CollectPointLights();
	std::optional<DirectionalLightData> dirLight = CollectDirectionalLight();

	Renderer::RenderSkybox(editorCamera);

	auto renderableView = m_Scene->GetRegistry().view<MeshRendererComponent, MaterialComponent, WorldTransformComponent>();
	for (auto entity : renderableView)
	{
		auto& mesh = m_Scene->GetComponent<MeshRendererComponent>(entity);
		auto& material = m_Scene->GetComponent<MaterialComponent>(entity);
		auto& transform = m_Scene->GetComponent<WorldTransformComponent>(entity);

		Renderer::Render(
			transform,
			mesh,
			material,
			editorCamera,
			pointLights,
			dirLight ? &(*dirLight) : nullptr
		);
	}

	for (Layer* layer : m_LayerStack)
		layer->OnRender();

	Renderer::EndFrame();
}

/**
 * @brief Collects all point lights from the scene.
 * @return A vector containing the data for all active point lights.
 */
std::vector<PointLightData> Engine::CollectPointLights() noexcept
{
	std::vector<PointLightData> lights;
	auto view = m_Scene->GetRegistry().view<PointLightComponent, WorldTransformComponent>();

	for (auto entity : view)
	{
		auto& light = m_Scene->GetComponent<PointLightComponent>(entity);
		auto& transform = m_Scene->GetComponent<WorldTransformComponent>(entity);

		PointLightData data;
		data.position = glm::vec3(transform.GetMatrix()[3]);
		data.color = light.color;
		data.intensity = light.intensity;
		data.radius = light.radius;

		lights.push_back(data);
	}

	return lights;
}

/**
 * @brief Collects the main directional light from the scene if available.
 * @return An optional containing the directional light data, or nullopt if no directional light is present.
 */
std::optional<DirectionalLightData> Engine::CollectDirectionalLight() noexcept
{
	auto view = m_Scene->GetRegistry().view<DirectionalLightComponent, WorldTransformComponent>();

	for (auto entity : view)
	{
		auto& light = m_Scene->GetComponent<DirectionalLightComponent>(entity);
		auto& transform = m_Scene->GetComponent<WorldTransformComponent>(entity);

		DirectionalLightData data;
		data.direction = transform.GetForward();
		data.color = light.color;
		data.intensity = light.intensity;
		data.castShadows = light.castShadows;

		return data;
	}

	return std::nullopt;
}

/**
 * @brief Shuts down the Engine and deinitializes all resources.
 * Cleans up the renderer and terminates GLFW.
 */
void Engine::Shutdown()
{
	if (m_Renderer)
		m_Renderer->Shutdown();

	glfwTerminate();
}
