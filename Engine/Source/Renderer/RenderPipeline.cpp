#include "rvelapch.h"
#include "RenderPipeline.h"
#include "Utils/GPUTimer.h"
#include "Core/Engine.h"
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

using namespace rv;

RenderPipeline::RenderPipeline()
{
	PushRenderPass(std::make_unique<GeometryPass>());
	PushRenderPass(std::make_unique<ShadowPass>());
	PushRenderPass(std::make_unique<EntityBufferPass>());
	PushRenderPass(std::make_unique<LightingPass>());
	PushRenderPass(std::make_unique<SkyboxPass>());
	PushRenderPass(std::make_unique<TransparentPass>());
	PushRenderPass(std::make_unique<SSAOPass>());
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

void RenderPipeline::Execute(const RenderContext& ctx)
{
	for (auto& [id, pass] : m_RenderPasses)
		pass->Execute(ctx, m_RenderFrame);
	GLenum afterErr = glGetError();

	m_RenderFrame.Reset();

}