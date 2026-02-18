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
#include "Rendering/RenderPasses/OutlinePass.h"
#include "Rendering/RenderPasses/EntityBufferPass.h"

namespace rv {

RenderPipeline::RenderPipeline(Engine* engine)
{
	m_RenderPasses.push_back(new GeometryPass());
	m_RenderPasses.push_back(new ShadowPass());
	m_RenderPasses.push_back(new EntityBufferPass());
	m_RenderPasses.push_back(new SkyboxPass());
	m_RenderPasses.push_back(new LightingPass());
	m_RenderPasses.push_back(new SSAOPass());
	m_RenderPasses.push_back(new SSRPass());
	m_RenderPasses.push_back(new SSRPass());
	m_RenderPasses.push_back(new BrightPass());
	m_RenderPasses.push_back(new BloomPass());
	m_RenderPasses.push_back(new CompositePass());

	this->engine = engine;
}

void RenderPipeline::EnsureInitialized(const RenderContext& ctx)
{
	if (!isInitialized)
	{
		for (auto* pass : m_RenderPasses)
			pass->Init(ctx,m_RenderFrame);

		isInitialized = true;
	}
}

void RenderPipeline::Execute(const RenderContext& ctx)
{

	for (auto* pass : m_RenderPasses)
		pass->Execute(ctx, m_RenderFrame);
	
	m_RenderFrame.Reset();

}

}