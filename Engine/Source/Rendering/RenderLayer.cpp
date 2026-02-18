#include "rvelapch.h"
#include "RenderLayer.h"
#include "Core/Engine.h"

namespace rv {


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

static std::optional<DirectionalLightData> CollectDirectionalLight(Scene& scene) noexcept {
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

		return data;
	}
	return std::nullopt;
}


void RenderLayer::OnRender()
{
	
	

	Scene& scene = m_Engine->GetActiveScene();
	ICamera* camera = m_Engine->GetCamera();

	
	m_Context.Clear();

	m_Context.camera = camera;
	m_Context.pointLights = CollectPointLights(scene);
	m_Context.directionalLight = CollectDirectionalLight(scene);
	m_Context.viewportWidth = 1920;
	m_Context.viewportHeight = 1080;
	m_Context.scene = &scene;
	
	m_RenderPipeline->SetRenderContext(m_Context);
	m_RenderPipeline->EnsureInitialized();

	
	// Collect commands and submit them to render passes via render pipeline
	CollectRenderCommands(&scene, [&](const RenderCommand& cmd) {
		m_RenderPipeline->SubmitRenderCommand(cmd);
		});
	
	m_RenderPipeline->Execute();
}

void RenderLayer::CollectRenderCommands(Scene* scene, const std::function<void(const RenderCommand&)>& submitCallback)
{

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
		submitCallback(cmd);
		
	}


}

}