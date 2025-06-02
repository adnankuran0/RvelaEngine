#include "rvelapch.h"
#include "RenderPipeline.h"

RenderPipeline::RenderPipeline()
{
	renderPasses = {
		&geometryPass,
		&shadowPass,
		&skyboxPass,
		&lightingPass,
		&brightPass,
		&bloomPass,
		&ssaoPass,
		&ssrPass,
		&compositePass
	};
}

void RenderPipeline::EnsureInitialized()
{
	if (!isInitialized)
	{
		for (auto* pass : renderPasses)
			pass->Init();



		isInitialized = true;
	}
}

void RenderPipeline::SetRenderContext(const RenderContext& context)
{
	for (auto* pass : renderPasses)
		pass->SetRenderContext(context);
}

//This function called for every RenderCommand in RenderLayer
void RenderPipeline::SubmitRenderCommand(const RenderCommand& cmd)
{
	shadowPass.AddRenderCommand(cmd);
	geometryPass.AddRenderCommand(cmd);
	lightingPass.AddRenderCommand(cmd);
}

void RenderPipeline::Execute()
{
	geometryPass.Execute();

	shadowPass.Execute();

	skyboxPass.screenFBO = lightingPass.GetScreenFBO();
	skyboxPass.Execute();

	lightingPass.SetDirectionalShadowMap(shadowPass.GetDirectionalShadowMap());
	lightingPass.SetPointShadowMap(shadowPass.GetPointShadowMap());
	lightingPass.SetLightSpaceMatrix(shadowPass.GetLightSpaceMatrix());
	lightingPass.Execute();

	ssaoPass.SetDepthTexture(geometryPass.GetDepthTexure());
	ssaoPass.SetNormalTexture(geometryPass.GetNormalTexure());
	ssaoPass.Execute();

	brightPass.SetScreenTexture(lightingPass.GetScreenTexture());
	brightPass.Execute();

	bloomPass.SetBrightTexture(brightPass.GetBrightTexture());
	bloomPass.Execute();

	ssrPass.SetDepthTexture(geometryPass.GetDepthTexure());
	ssrPass.SetNormalTexture(geometryPass.GetNormalTexure());
	ssrPass.SetRoughnessTexture(geometryPass.GetRoughnessTexure());
	ssrPass.SetMetallicTexture(geometryPass.GetMetallicTexure());
	ssrPass.SetScreenTexture(lightingPass.GetScreenTexture());
	ssrPass.Execute();

	compositePass.SetScreenTexture(lightingPass.GetScreenTexture());
	compositePass.SetBloomBlurTexture(bloomPass.GetBloomBlurTexture());
	compositePass.SetAoTexture(ssaoPass.GetSSAOTexture());
	compositePass.SetSsrTexture(ssrPass.GetSSRTexture());
	compositePass.Execute();
}
