#pragma once
#include <vector>
#include <memory>
#include "RenderPass.h"
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
#include "Core/Engine.h"

struct RenderFeatures
{
	bool ssao = true;
	bool ssr = false;
	bool bloom = true;
};


class RenderPipeline
{
public:

	RenderPipeline(Engine* engine);
	void EnsureInitialized();
	void SetRenderContext(const RenderContext& context);
	void SubmitRenderCommand(const RenderCommand& cmd);
	[[nodiscard]] inline GLuint GetFinalTexture() noexcept { return compositePass.GetFinalTexture(); }
	void Execute();

private:
	Engine* engine;

	RenderFeatures m_RenderFeatures;
	bool isPipelineDirty = true;

	bool isInitialized = false;
	//Render Passes
	std::vector<RenderPass*> renderPasses;
	GeometryPass geometryPass;
	ShadowPass shadowPass;
	SkyboxPass skyboxPass;
	LightingPass lightingPass;
	BrightPass brightPass;
	BloomPass bloomPass;
	SSAOPass ssaoPass;
	SSRPass ssrPass;
	CompositePass compositePass;

};

