#include "rvelapch.h"
#include "RenderLayer.h"
#include "Core/Engine.h"
#include "Light.h"
#include "glm/gtx/norm.hpp"

using namespace rv;

static const SkeletonComponent* FindSkeletonInHierarchy(Scene* scene, entt::entity entity)
{
	auto& reg = scene->GetRegistry();

	if (reg.all_of<SkeletonComponent>(entity))
		return &reg.get<SkeletonComponent>(entity);

	entt::entity current = entity;
	while (reg.valid(current) && reg.all_of<SceneTreeComponent>(current))
	{
		const auto& tree = reg.get<SceneTreeComponent>(current);
		if (tree.parent == entt::null || !reg.valid(tree.parent))
			break;

		current = tree.parent;
		if (reg.all_of<SkeletonComponent>(current))
			return &reg.get<SkeletonComponent>(current);
	}

	return nullptr;
}

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

	auto view = scene->GetRegistry().view<TransformComponent, MeshComponent, MeshRendererComponent, MaterialComponent> (entt::exclude<ParticleEmitterComponent>);
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

		RenderCommand cmd;
		cmd.transform = &scene->GetComponent<TransformComponent>(entity);
		cmd.mesh = &scene->GetComponent<MeshRendererComponent>(entity);
		cmd.material = &scene->GetComponent<MaterialComponent>(entity);
		cmd.entityID = entity;

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

	auto skeletalView = scene->GetRegistry().view<TransformComponent, SkeletalMeshComponent, SkeletalMeshRendererComponent, MaterialComponent>(entt::exclude<ParticleEmitterComponent>);
	for (auto entity : skeletalView)
	{
		SkeletalMeshComponent& meshComp = scene->GetComponent<SkeletalMeshComponent>(entity);
		if (meshComp.IsDirty())
		{
			scene->GetComponent<SkeletalMeshRendererComponent>(entity).RecreateFromMesh(meshComp.GetMesh());
			meshComp.SetDirty(false);
		}

		TransformComponent& transform = scene->GetComponent<TransformComponent>(entity);
		MaterialComponent& matComp = scene->GetComponent<MaterialComponent>(entity);
		const SkeletonComponent* skeletonComp = FindSkeletonInHierarchy(scene, entity);
		if (!skeletonComp)
			continue;

		SkeletalRenderCommand cmd;
		cmd.transform = &scene->GetComponent<TransformComponent>(entity);
		cmd.mesh = &scene->GetComponent<SkeletalMeshRendererComponent>(entity);
		cmd.material = &scene->GetComponent<MaterialComponent>(entity);
		cmd.entityID = entity;
		cmd.skeleton = skeletonComp;

		if (matComp.GetTransparencyMode() == TransparencyMode::Alpha)
		{
			glm::vec3 objPos = glm::vec3(transform.GetWorldMatrix()[3]);
			cmd.distanceToCamera = glm::length2(camPos - objPos);
			m_RenderPipeline->m_RenderFrame.skeletalTransparentCommands.push_back(cmd);
		}
		else
		{
			m_RenderPipeline->m_RenderFrame.skeletalOpaqueCommands.push_back(cmd);
		}
	}
	std::sort(
		m_RenderPipeline->m_RenderFrame.skeletalTransparentCommands.begin(),
		m_RenderPipeline->m_RenderFrame.skeletalTransparentCommands.end(),
		[](const SkeletalRenderCommand& a, const SkeletalRenderCommand& b) {
			return a.distanceToCamera > b.distanceToCamera;
		}
	);

	const auto& particleBatches = scene->GetParticleSystem().GetBatches();
	for (const auto& batch : particleBatches)
	{
		if (scene->GetRegistry().all_of<MeshComponent, MeshRendererComponent, MaterialComponent>(batch.entity))
		{
			MeshComponent& meshComp = scene->GetComponent<MeshComponent>(batch.entity);
			MeshRendererComponent& meshRendererComp = scene->GetComponent<MeshRendererComponent>(batch.entity);
			MaterialComponent& matComp = scene->GetComponent<MaterialComponent>(batch.entity);

			if (meshComp.IsDirty())
			{
				scene->GetComponent<MeshRendererComponent>(batch.entity).RecreateFromMesh(meshComp.GetMesh());
				meshComp.SetDirty(false);
			}

			ParticleRenderCommand cmd;
			cmd.mesh = &meshRendererComp;
			cmd.material = &matComp;
			cmd.instanceOffset = batch.instanceOffset;
			cmd.instanceCount = batch.instanceCount;
			cmd.localCoords = batch.localCoords;
			cmd.worldPosition = batch.worldPosition;
			cmd.indexCount = meshComp.GetMesh()->GetIndexCount();
			cmd.distanceToCamera = glm::length2(camPos - batch.worldPosition);

			m_RenderPipeline->m_RenderFrame.particleCommands.push_back(cmd);
		}
	}

	std::sort(
		m_RenderPipeline->m_RenderFrame.particleCommands.begin(),
		m_RenderPipeline->m_RenderFrame.particleCommands.end(),
		[](const ParticleRenderCommand& a, const ParticleRenderCommand& b) {
			return a.distanceToCamera > b.distanceToCamera;
		}
	);

}

