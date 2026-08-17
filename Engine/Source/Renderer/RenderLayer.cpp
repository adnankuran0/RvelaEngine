#include "rvelapch.h"
#include "RenderLayer.h"
#include "Core/Engine.h"
#include "Light.h"
#include "glm/gtx/norm.hpp"

using namespace rv;

void RenderLayer::OnRender()
{

	Scene& scene = m_Engine->GetActiveScene();
	Camera* camera = m_Engine->GetCamera();
	LightSystem& lightSystem = scene.GetLightSystem();
	
	m_Context.Clear();

	m_Context.camera = camera;
	m_Context.pointLights = lightSystem.CollectPointLights();
	m_Context.directionalLight = lightSystem.CollectDirectionalLight(camera->Position);
	m_Context.viewportWidth = 1920;
	m_Context.viewportHeight = 1080;
	m_Context.scene = &scene;
	m_Context.environment = &scene.GetEnvironment();
	
	m_RenderPipeline->EnsureInitialized(m_Context);
	
	CollectRenderCommands(&scene);
	
	m_RenderPipeline->Execute(m_Context);
}

void RenderLayer::CollectRenderCommands(Scene* scene)
{

	Camera* camera = Engine::Get()->GetCamera();
	glm::vec3 camPos = camera->Position;

	auto view = scene->GetRegistry().view<TransformComponent, MeshComponent ,MeshRendererComponent, MaterialComponent>();
	for (auto entity : view)
	{
		MeshComponent& meshComp = scene->GetComponent<MeshComponent>(entity);
		if (meshComp.IsDirty())
		{
			scene->GetComponent<MeshRendererComponent>(entity).RecreateFromMesh(meshComp.GetMesh());
			meshComp.SetDirty(false);
		}
		
		TransformComponent& transform = scene->GetComponent<TransformComponent>(entity);
		MaterialComponent& matComp = scene->GetComponent<MaterialComponent>(entity);

		RenderCommand cmd(scene->GetComponent<TransformComponent>(entity),
			scene->GetComponent<MeshRendererComponent>(entity),
			scene->GetComponent<MaterialComponent>(entity),
			entity);

		if (matComp.GetTransparencyMode() == TransparencyMode::Alpha)
		{
			glm::vec3 objPos = glm::vec3(transform.GetWorldMatrix()[3]);
			cmd.distanceToCamera = glm::length2(camPos - objPos);
			m_RenderPipeline->m_RenderFrame.transparentCommands.push_back(cmd);
		}
		else
		{
			m_RenderPipeline->m_RenderFrame.opaqueCommands.push_back(cmd);
		}
	}
	std::sort(
		m_RenderPipeline->m_RenderFrame.transparentCommands.begin(),
		m_RenderPipeline->m_RenderFrame.transparentCommands.end(),
		[](const RenderCommand& a, const RenderCommand& b) {
			return a.distanceToCamera > b.distanceToCamera;
		}
	);

}

