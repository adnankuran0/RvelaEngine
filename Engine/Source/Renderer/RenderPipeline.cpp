#include "rvelapch.h"
#include "RenderPipeline.h"
#include "Utils/GPUTimer.h"
#include "Core/Engine.h"
#include "RenderContext.h"
#include "Renderer/RenderPasses/LightingPass.h"
#include "Renderer/RenderPasses/SkyboxPass.h"
#include "Renderer/RenderPasses/ShadowPass.h"
#include "Renderer/RenderPasses/CompositePass.h"
#include "Renderer/RenderPasses/BrightPass.h"
#include "Renderer/RenderPasses/BloomPass.h"
#include "Renderer/RenderPasses/GeometryPass.h"
#include "Renderer/RenderPasses/SSAOPass.h"
#include "Renderer/RenderPasses/SSRPass.h"
#include "Renderer/RenderPasses/EntityBufferPass.h"
#include "Renderer/RenderPasses/DebugPass.h"
#include "Renderer/RenderPasses/TransparentPass.h"
#include "Renderer/RenderPasses/ParticlePass.h"

using namespace rv;

RenderPipeline::RenderPipeline()
{
	m_CameraUBO = std::make_unique<UniformBuffer>(sizeof(CameraUBOData), 0);
	m_LightUBO = std::make_unique<UniformBuffer>(sizeof(LightUBOData), 1);

	PushRenderPass(std::make_unique<GeometryPass>());
	PushRenderPass(std::make_unique<ShadowPass>());
	PushRenderPass(std::make_unique<EntityBufferPass>());
	PushRenderPass(std::make_unique<SSAOPass>());
	PushRenderPass(std::make_unique<LightingPass>());
	PushRenderPass(std::make_unique<SkyboxPass>());
	PushRenderPass(std::make_unique<TransparentPass>());
	PushRenderPass(std::make_unique<ParticlePass>());
	PushRenderPass(std::make_unique<SSRPass>());
	PushRenderPass(std::make_unique<BrightPass>());
	PushRenderPass(std::make_unique<BloomPass>());
	PushRenderPass(std::make_unique<CompositePass>());
	PushRenderPass(std::make_unique<DebugPass>());
}

void RenderPipeline::EnsureInitialized(const RenderContext& ctx)
{
	if (!isInitialized)
	{
		for (auto& [id, pass] : m_RenderPasses)
			pass->Init(ctx, m_RenderFrame);

		isInitialized = true;
	}
}

RenderPassHandle RenderPipeline::PushRenderPass(std::unique_ptr<RenderPass> pass)
{
	RenderPassHandle id = m_NextID++;
	m_RenderPasses.push_back({ id, std::move(pass) });
	return id;
}

RenderPass* RenderPipeline::GetRenderPass(RenderPassHandle id)
{
	for (auto& entry : m_RenderPasses)
	{
		if (entry.id == id)
			return entry.pass.get();
	}
	return nullptr;
}

void RenderPipeline::UpdateUBOs(const RenderContext& ctx)
{
	if (!ctx.camera)
		return;

	CameraUBOData camData;
	camData.view = ctx.camera->GetViewMatrix();
	camData.projection = ctx.camera->GetProjectionMatrix();
	camData.invView = glm::inverse(camData.view);
	camData.invProjection = glm::inverse(camData.projection);
	camData.camPos = ctx.camera->Position;
	camData.padding0 = 0.0f;
	camData.windowSize = glm::vec2(static_cast<float>(ctx.viewportWidth), static_cast<float>(ctx.viewportHeight));
	camData.nearPlane = ctx.camera->NearClip;
	camData.farPlane = ctx.camera->FarClip;

	m_CameraUBO->SetData(0, sizeof(CameraUBOData), &camData);

	LightUBOData lightData{};

	if (ctx.directionalLight.has_value())
	{
		const auto& dir = ctx.directionalLight.value();
		lightData.hasDirLight = 1;
		lightData.dirLight.direction = glm::vec4(dir.direction, dir.castShadows ? 1.0f : 0.0f);
		lightData.dirLight.colorIntensity = glm::vec4(dir.color, dir.intensity);
		lightData.dirLight.shadowBias = dir.shadowBias;
		lightData.dirLight.blurRadius = dir.blurRadius;
		lightData.lightSpaceMatrix = dir.lightSpace;
	}
	else
	{
		lightData.hasDirLight = 0;
	}

	int count = std::min(static_cast<int>(ctx.pointLights.size()), 20);
	lightData.pointLightCount = count;

	for (int i = 0; i < count; ++i)
	{
		const auto& pl = ctx.pointLights[i];
		lightData.pointLights[i].position = glm::vec4(pl.position, pl.falloff);
		lightData.pointLights[i].colorIntensity = glm::vec4(pl.color, pl.intensity);
		lightData.pointLights[i].radius = pl.radius;
		lightData.pointLights[i].shadowBias = pl.shadowBias;
		lightData.pointLights[i].blurRadius = pl.blurRadius;
		lightData.pointLights[i].shadowIndex = pl.castShadows ? pl.shadowIndex : -1;
	}

	m_LightUBO->SetData(0, sizeof(LightUBOData), &lightData);

	// TODO: environment UBO

}

void RenderPipeline::Execute(const RenderContext& ctx)
{
	UpdateUBOs(ctx);

	for (auto& [id, pass] : m_RenderPasses)
		pass->Execute(ctx, m_RenderFrame);
	GLenum afterErr = glGetError();

	m_RenderFrame.Reset();

}