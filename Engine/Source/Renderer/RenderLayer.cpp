#include "rvelapch.h"
#include "RenderLayer.h"
#include "Core/Engine.h"
#include "Light.h"
#include "glm/gtx/norm.hpp"
#include "Asset/Types/ShaderAsset.h"

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
	if (!camera)
		return;

	LightSystem& lightSystem = scene.GetLightSystem();
	
	m_Context.Clear();

	m_Context.camera = camera;
	m_Context.pointLights = lightSystem.CollectPointLights();
	m_Context.directionalLight = lightSystem.CollectDirectionalLight(camera);

	uint32_t renderW = 1920;
	uint32_t renderH = 1080;

	if (ProjectManager::IsProjectLoaded())
	{
		const auto& settings = ProjectManager::GetActiveProject()->GetSettings();
		if (m_TargetWidth > 0 && m_TargetHeight > 0)
		{
			renderW = m_TargetWidth;
			renderH = m_TargetHeight;
		}
		else if (settings.viewportWidth > 0 && settings.viewportHeight > 0)
		{
			renderW = settings.viewportWidth;
			renderH = settings.viewportHeight;
		}
	}
	else
	{
		if (m_TargetWidth > 0 && m_TargetHeight > 0)
		{
			renderW = m_TargetWidth;
			renderH = m_TargetHeight;
		}
	}

	m_Context.viewportWidth = renderW;
	m_Context.viewportHeight = renderH;
	m_Context.scene = &scene;
	m_Context.environment = &scene.GetEnvironment();
	m_Context.wireframe = m_Wireframe;

	if (camera)
	{
		camera->width = static_cast<int>(renderW);
		camera->height = static_cast<int>(renderH);
	}
	
	m_RenderPipeline->EnsureInitialized(m_Context);
	
	CollectRenderCommands(&scene);
	
	m_RenderPipeline->Execute(m_Context);

	GLuint finalFB = m_RenderPipeline->GetFinalFramebuffer();
	GLuint depthTexture = m_RenderPipeline->GetDepthTexture();

	if (finalFB != 0)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, finalFB);
		glViewport(0, 0, m_Context.viewportWidth, m_Context.viewportHeight);

		if (depthTexture != 0)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
		}
	}

	scene.GetUISystem().Render(&scene, (float)m_Context.viewportWidth, (float)m_Context.viewportHeight);

	if (finalFB != 0)
	{
		if (depthTexture != 0)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void RenderLayer::CollectRenderCommands(Scene* scene)
{

	Camera* camera = Engine::Get()->GetCamera();
	glm::vec3 camPos = camera->Position;

	auto view = scene->GetRegistry().view<TransformComponent, MeshComponent, MeshRendererComponent, MaterialComponent> (entt::exclude<ParticleEmitterComponent>);
	for (auto entity : view)
	{
		if (!scene->IsEntityActive(entity))
			continue;

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

		bool isTransparent = (matComp.GetTransparencyMode() == TransparencyMode::Alpha);
		auto customShader = matComp.GetInstance().GetCustomShaderAsset();
		if (customShader && customShader->IsValid())
		{
			if (customShader->renderModes.hasBlendMode || customShader->renderModes.depthDrawDisabled)
			{
				isTransparent = true;
			}
		}

		if (isTransparent)
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
		if (!scene->IsEntityActive(entity))
			continue;

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

		bool isSkeletalTransparent = (matComp.GetTransparencyMode() == TransparencyMode::Alpha);
		auto skeletalCustomShader = matComp.GetInstance().GetCustomShaderAsset();
		if (skeletalCustomShader && skeletalCustomShader->IsValid())
		{
			if (skeletalCustomShader->renderModes.hasBlendMode || skeletalCustomShader->renderModes.depthDrawDisabled)
			{
				isSkeletalTransparent = true;
			}
		}

		if (isSkeletalTransparent)
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

	auto decalView = scene->GetRegistry().view<TransformComponent, DecalComponent>();
	for (auto entity : decalView)
	{
		if (!scene->IsEntityActive(entity))
			continue;

		const auto& transform = scene->GetComponent<TransformComponent>(entity);
		const auto& decal = scene->GetComponent<DecalComponent>(entity);

		DecalRenderCommand cmd;
		cmd.entityID = entity;
		cmd.transform = &transform;
		cmd.decal = &decal;
		cmd.distanceToCamera = glm::length2(camPos - transform.GetWorldPosition());

		m_RenderPipeline->m_RenderFrame.decalCommands.push_back(cmd);
	}
}

