#include "rvelapch.h"
#include "RenderLayer.h"
#include "Core/Engine.h"


using namespace rv;

static std::vector<PointLightData> CollectPointLights(Scene& scene) noexcept {
	std::vector<PointLightData> lights;
	auto view = scene.GetRegistry().view<PointLightComponent, TransformComponent>();
	for (auto e : view) {
		auto& light = scene.GetComponent<PointLightComponent>(e);
		auto& t = scene.GetComponent<TransformComponent>(e);
		PointLightData data;
		data.position = t.GetWorldPosition();
		data.color = light.color;
		data.intensity = light.intensity;
		data.radius = light.radius;
		data.falloff = light.falloff;
		data.castShadows = light.castShadows;
		data.shadowIndex = light.shadowIndex;
		data.shadowBias = light.shadowBias;
		data.reverseCullFace = light.reverseCullFace;
		data.blurRadius = light.blurRadius;
		lights.push_back(data);
	}
	return lights;
}

static std::optional<DirectionalLightData> CollectDirectionalLight(Scene& scene,glm::vec3 cameraPos) noexcept {
	auto view = scene.GetRegistry().view<DirectionalLightComponent, TransformComponent>();
	for (auto e : view) {
		auto& light = scene.GetComponent<DirectionalLightComponent>(e);
		auto& t = scene.GetComponent<TransformComponent>(e);
		DirectionalLightData data;
		data.direction = t.GetForward();
		data.color = light.color;
		data.intensity = light.intensity;
		data.shadowBias = light.shadowBias;
		data.castShadows = light.castShadows;
		data.reverseCullFace = light.reverseCullFace;
		data.blurRadius = light.blurRadius;

		glm::vec3 lightDir = data.direction;
		glm::vec3 sceneCenter = cameraPos;
		glm::mat4 lightView = glm::lookAt(sceneCenter - lightDir * 50.0f, sceneCenter, glm::vec3(0, 1, 0));
		float orthoSize = 35.0f;
		glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 120.0f);
		data.lightSpace = lightProjection * lightView;

		return data;
	}
	return std::nullopt;
}


void RenderLayer::OnRender()
{

	Scene& scene = m_Engine->GetActiveScene();
	Camera* camera = m_Engine->GetCamera();

	
	m_Context.Clear();

	m_Context.camera = camera;
	m_Context.pointLights = CollectPointLights(scene);
	m_Context.directionalLight = CollectDirectionalLight(scene,camera->Position);
	m_Context.viewportWidth = 1920;
	m_Context.viewportHeight = 1080;
	m_Context.scene = &scene;
	
	m_RenderPipeline->EnsureInitialized(m_Context);
	
	CollectRenderCommands(&scene);
	
	m_RenderPipeline->Execute(m_Context);
}

void RenderLayer::CollectRenderCommands(Scene* scene)
{
	m_RenderPipeline->m_RenderFrame.commands.clear();

	auto view = scene->GetRegistry().view<TransformComponent, MeshComponent ,MeshRendererComponent, MaterialComponent>();
	for (auto entity : view)
	{

		MeshComponent& meshComp = scene->GetComponent<MeshComponent>(entity);
		
		if (meshComp.IsDirty())
		{
			scene->GetComponent<MeshRendererComponent>(entity).RecreateFromMesh(meshComp.GetMesh());
			meshComp.SetDirty(false);
		}
		
		RenderCommand cmd(scene->GetComponent<TransformComponent>(entity),
			scene->GetComponent<MeshRendererComponent>(entity),
			scene->GetComponent<MaterialComponent>(entity),
			entity);

		m_RenderPipeline->m_RenderFrame.commands.push_back(cmd);
		
	}


}

