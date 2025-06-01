#pragma once
#include <vector>
#include <memory>
#include "RenderPass.h"
#include "Renderer/Passes/LightingPass.h"
#include "Renderer/Passes/SkyboxPass.h"
#include "Renderer/Passes/ShadowPass.h"
#include "Renderer/Passes/CompositePass.h"
#include "Renderer/Passes/BrightPass.h"
#include "Renderer/Passes/BloomPass.h"
#include "Renderer/Passes/GeometryPass.h"
#include "Renderer/Passes/SSAOPass.h"
#include "Renderer/Passes/SSRPass.h"

class RenderPipeline
{
public:

	RenderPipeline();
	void EnsureInitialized();
	void SetRenderContext(const RenderContext& context);
	void SubmitRenderCommand(const RenderCommand& cmd);
	void Execute();

private:
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

