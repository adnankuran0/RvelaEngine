#include "rvelapch.h"
#include "RenderLayer.h"
#include "Core/Engine.h"
#include "Light.h"
#include "glm/gtx/norm.hpp"
#include "Asset/Types/ShaderAsset.h"
#include "Scene/Components/VisibilityRangeComponent.h"
#include <algorithm>

using namespace rv;

static bool EvaluateVisibilityRange(const VisibilityRangeComponent* vis, float dist, float& outFade)
{
	outFade = 1.0f;
	if (!vis)
		return true;

	// End check
	if (vis->end > 0.0f)
	{
		if (dist > vis->end)
			return false;

		if (vis->fade && vis->endMargin > 0.0f)
		{
			float fadeStart = std::max(0.0f, vis->end - vis->endMargin);
			if (dist > fadeStart)
			{
				outFade = std::clamp((vis->end - dist) / vis->endMargin, 0.0f, 1.0f);
			}
		}
	}

	// Begin check
	if (vis->begin > 0.0f)
	{
		float minVisibleDist = vis->begin;
		if (vis->fade && vis->beginMargin > 0.0f)
			minVisibleDist = std::max(0.0f, vis->begin - vis->beginMargin);

		if (dist < minVisibleDist)
			return false;

		if (vis->fade && vis->beginMargin > 0.0f && dist < vis->begin)
		{
			float beginFade = std::clamp((dist - (vis->begin - vis->beginMargin)) / vis->beginMargin, 0.0f, 1.0f);
			outFade = std::min(outFade, beginFade);
		}
	}

	return true;
}

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

static const VisibilityRangeComponent* FindVisibilityRangeInHierarchy(Scene* scene, entt::entity entity, glm::vec3& inOutPivotPos)
{
	auto& reg = scene->GetRegistry();

	if (reg.all_of<VisibilityRangeComponent>(entity))
		return &reg.get<VisibilityRangeComponent>(entity);

	entt::entity current = entity;
	while (reg.valid(current) && reg.all_of<SceneTreeComponent>(current))
	{
		const auto& tree = reg.get<SceneTreeComponent>(current);
		if (tree.parent == entt::null || !reg.valid(tree.parent))
			break;

		current = tree.parent;
		if (reg.all_of<VisibilityRangeComponent>(current))
		{
			if (reg.all_of<TransformComponent>(current))
				inOutPivotPos = glm::vec3(reg.get<TransformComponent>(current).GetWorldMatrix()[3]);

			return &reg.get<VisibilityRangeComponent>(current);
		}
	}

	return nullptr;
}

void RenderLayer::OnRender()
{
	Scene& scene = m_Engine->GetActiveScene();
	Camera* camera = m_Engine->GetCamera();

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

	m_RenderPipeline->EnsureInitialized(m_Context);

	if (camera)
	{
		camera->width = static_cast<int>(renderW);
		camera->height = static_cast<int>(renderH);

		LightSystem& lightSystem = scene.GetLightSystem();
		m_Context.Clear();
		m_Context.camera = camera;
		m_Context.pointLights = lightSystem.CollectPointLights();
		m_Context.spotLights = lightSystem.CollectSpotLights();
		m_Context.directionalLight = lightSystem.CollectDirectionalLight(camera);
		m_Context.viewportWidth = renderW;
		m_Context.viewportHeight = renderH;
		m_Context.scene = &scene;
		m_Context.environment = &scene.GetEnvironment();
		m_Context.wireframe = m_Wireframe;

		CollectRenderCommands(&scene);
		m_RenderPipeline->Execute(m_Context);
	}
	else
	{
		m_Context.Clear();
		GLuint finalFB = m_RenderPipeline->GetFinalFramebuffer();
		if (finalFB != 0)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, finalFB);
			glViewport(0, 0, m_Context.viewportWidth, m_Context.viewportHeight);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

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
		auto& meshRenderer = scene->GetComponent<MeshRendererComponent>(entity);
		if (meshComp.IsDirty() || meshRenderer.indexCount == 0)
		{
			meshRenderer.RecreateFromMesh(meshComp.GetMesh());
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

		glm::vec3 objPos = glm::vec3(transform.GetWorldMatrix()[3]);
		cmd.distanceToCamera = glm::length2(camPos - objPos);

		//Calculate LOD based on screen size ratio
		uint32_t chosenLOD = 0;
		uint32_t chosenIndexOffset = 0;
		uint32_t chosenIndexCount = meshRenderer.indexCount;

		if (meshRenderer.gpuMesh && !meshRenderer.gpuMesh->lods.empty())
		{
			const auto& lods = meshRenderer.gpuMesh->lods;
			if (meshRenderer.enableAutoLOD && lods.size() > 1)
			{
				glm::vec3 meshCenter = meshRenderer.worldAABB.IsValid()
					? meshRenderer.worldAABB.Center()
					: objPos;
				float linearDist = glm::distance(camPos, meshCenter);
				linearDist = std::max(linearDist, 0.001f);

				float radius = meshRenderer.worldAABB.IsValid()
					? glm::length(meshRenderer.worldAABB.Size()) * 0.5f
					: 1.0f;
				radius = std::max(radius, 0.001f);

				float projFactor = 1.0f;
				if (camera->ProjectionType == Camera::Projection::Perspective)
				{
					float fovRad = glm::radians(camera->FOV);
					projFactor = 1.0f / (2.0f * std::tan(fovRad * 0.5f));
				}
				else
				{
					projFactor = 1.0f / std::max(camera->OrthoSize, 0.001f);
				}

				float combinedBias = std::max(camera->LODBias * meshRenderer.lodBias, 0.001f);

				float screenRatio = 0.0f;
				if (camera->ProjectionType == Camera::Projection::Perspective)
				{
					screenRatio = (2.0f * radius / linearDist) * projFactor * combinedBias;
				}
				else
				{
					screenRatio = (2.0f * radius) * projFactor * combinedBias;
				}

				chosenLOD = 0;
				for (size_t i = 1; i < lods.size(); ++i)
				{
					if (screenRatio < lods[i].screenRatio)
					{
						chosenLOD = static_cast<uint32_t>(i);
					}
					else
					{
						break;
					}
				}
			}

			meshRenderer.currentLOD = chosenLOD;
			chosenIndexOffset = lods[chosenLOD].indexOffset;
			chosenIndexCount = lods[chosenLOD].indexCount;
		}

		cmd.lodIndex = chosenLOD;
		cmd.indexOffset = chosenIndexOffset;
		cmd.indexCount = chosenIndexCount;

		glm::vec3 visPivotPos = objPos;
		if (const auto* vis = FindVisibilityRangeInHierarchy(scene, entity, visPivotPos))
		{
			float linearDist = glm::distance(camPos, visPivotPos);
			if (!EvaluateVisibilityRange(vis, linearDist, cmd.visibilityFade))
				continue;
		}

		if (isTransparent)
		{
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
	std::sort(
		m_RenderPipeline->m_RenderFrame.opaqueCommands.begin(),
		m_RenderPipeline->m_RenderFrame.opaqueCommands.end(),
		[](const RenderCommand& a, const RenderCommand& b) {
			return a.distanceToCamera < b.distanceToCamera;
		}
	);

	auto skeletalView = scene->GetRegistry().view<TransformComponent, SkeletalMeshComponent, SkeletalMeshRendererComponent, MaterialComponent>(entt::exclude<ParticleEmitterComponent>);
	for (auto entity : skeletalView)
	{
		if (!scene->IsEntityActive(entity))
			continue;

		SkeletalMeshComponent& meshComp = scene->GetComponent<SkeletalMeshComponent>(entity);
		auto& skelRenderer = scene->GetComponent<SkeletalMeshRendererComponent>(entity);
		if (meshComp.IsDirty() || skelRenderer.indexCount == 0)
		{
			skelRenderer.RecreateFromMesh(meshComp.GetMesh());
			meshComp.SetDirty(false);
		}

		TransformComponent& transform = scene->GetComponent<TransformComponent>(entity);
		MaterialComponent& matComp = scene->GetComponent<MaterialComponent>(entity);
		const SkeletonComponent* skeletonComp = FindSkeletonInHierarchy(scene, entity);
		if (!skeletonComp)
			continue;

		if (auto meshAsset = meshComp.GetMesh())
		{
			if (skeletonComp && !skeletonComp->skinningPalette.empty())
			{
				AABB animatedAABB = meshAsset->CalculateAnimatedAABB(skeletonComp->skinningPalette);
				skelRenderer.worldAABB = animatedAABB.CalculateWorldAABB(transform.GetWorldMatrix());
			}
			else
			{
				skelRenderer.worldAABB = skelRenderer.localAABB.CalculateWorldAABB(transform.GetWorldMatrix());
			}
		}

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

		glm::vec3 objPos = glm::vec3(transform.GetWorldMatrix()[3]);
		cmd.distanceToCamera = glm::length2(camPos - objPos);

		glm::vec3 visPivotPos = objPos;
		if (const auto* vis = FindVisibilityRangeInHierarchy(scene, entity, visPivotPos))
		{
			float linearDist = glm::distance(camPos, visPivotPos);
			if (!EvaluateVisibilityRange(vis, linearDist, cmd.visibilityFade))
				continue;
		}

		if (isSkeletalTransparent)
		{
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
	std::sort(
		m_RenderPipeline->m_RenderFrame.skeletalOpaqueCommands.begin(),
		m_RenderPipeline->m_RenderFrame.skeletalOpaqueCommands.end(),
		[](const SkeletalRenderCommand& a, const SkeletalRenderCommand& b) {
			return a.distanceToCamera < b.distanceToCamera;
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
		auto& decal = scene->GetComponent<DecalComponent>(entity);

		if (!decal.worldAABB.IsValid())
		{
			decal.worldAABB = decal.localAABB.CalculateWorldAABB(transform.GetWorldMatrix());
		}

		if (camera && !camera->Intersects(decal.worldAABB))
			continue;

		DecalRenderCommand cmd;
		cmd.entityID = entity;
		cmd.transform = &transform;
		cmd.decal = &decal;
		cmd.distanceToCamera = glm::length2(camPos - transform.GetWorldPosition());
		cmd.worldAABB = decal.worldAABB;

		m_RenderPipeline->m_RenderFrame.decalCommands.push_back(cmd);
	}
}

