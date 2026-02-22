#include "rvelapch.h"
#include "RenderPipeline.h"
#include "Utils/GPUTimer.h"
#include "Core/Engine.h"
#include "Rendering/RenderPasses/LightingPass.h"
#include "Rendering/RenderPasses/SkyboxPass.h"
#include "Rendering/RenderPasses/ShadowPass.h"
#include "Rendering/RenderPasses/CompositePass.h"
#include "Rendering/RenderPasses/BrightPass.h"
#include "Rendering/RenderPasses/BloomPass.h"
#include "Rendering/RenderPasses/GeometryPass.h"
#include "Rendering/RenderPasses/SSAOPass.h"
#include "Rendering/RenderPasses/SSRPass.h"
#include "Rendering/RenderPasses/EntityBufferPass.h"

using namespace rv;

RenderPipeline::RenderPipeline()
{
	PushRenderPass(std::make_unique<GeometryPass>());
	PushRenderPass(std::make_unique<ShadowPass>());
	PushRenderPass(std::make_unique<EntityBufferPass>());
	PushRenderPass(std::make_unique<SkyboxPass>());
	PushRenderPass(std::make_unique<LightingPass>());
	PushRenderPass(std::make_unique<SSAOPass>());
	PushRenderPass(std::make_unique<SSRPass>());
	PushRenderPass(std::make_unique<BrightPass>());
	PushRenderPass(std::make_unique<BloomPass>());
	PushRenderPass(std::make_unique<CompositePass>());
}

void RenderPipeline::EnsureInitialized(const RenderContext& ctx)
{
	if (!isInitialized)
	{
		for (auto& [id, pass] : m_RenderPasses)
			pass->Init(ctx,m_RenderFrame);

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
	
	m_RenderFrame.Reset();

}