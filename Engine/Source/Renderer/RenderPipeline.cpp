#include "rvelapch.h"
#include "RenderPipeline.h"

RenderPipeline::RenderPipeline()
{
	renderPasses = {
		&geometryPass,
		&shadowPass,
		&skyboxPass,
		&lightingPass,
		&outlinePass,
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

	ssaoPass.SetDepthTexture(geometryPass.GetDepthTexure());
	ssaoPass.SetNormalTexture(geometryPass.GetNormalTexure());
	ssaoPass.Execute();

	shadowPass.Execute();

	skyboxPass.SetScreenFBO(lightingPass.GetScreenFBO());
	skyboxPass.Execute();

	lightingPass.SetDirectionalShadowMap(shadowPass.GetDirectionalShadowMap());
	lightingPass.SetPointShadowMap(shadowPass.GetPointShadowMap());
	lightingPass.SetLightSpaceMatrix(shadowPass.GetLightSpaceMatrix());
	lightingPass.Execute();

	//outlinePass.SetScreenFBO(lightingPass.GetScreenFBO());
	//outlinePass.SetIntermediateFBO(lightingPass.GetIntermediateFBO());
	//outlinePass.Execute();

	brightPass.SetScreenTexture(lightingPass.GetScreenTexture());
	brightPass.Execute();

	bloomPass.SetBrightTexture(brightPass.GetBrightTexture());
	bloomPass.Execute();

	ssrPass.SetDepthTexture(geometryPass.GetDepthTexure());
	ssrPass.SetNormalTexture(geometryPass.GetNormalTexure());
	ssrPass.SetRoughnessTexture(geometryPass.GetRoughnessTexure());
	ssrPass.SetMetallicTexture(geometryPass.GetMetallicTexure());
	ssrPass.SetScreenTexture(lightingPass.GetScreenTexture());
	ssrPass.SetSkyboxTexture(skyboxPass.GetSkyboxTexture());
	ssrPass.Execute();

	compositePass.SetScreenTexture(lightingPass.GetScreenTexture());
	compositePass.SetBloomBlurTexture(bloomPass.GetBloomBlurTexture());
	compositePass.SetAoTexture(ssaoPass.GetSSAOTexture());
	compositePass.SetSsrTexture(ssrPass.GetSSRTexture());
	compositePass.Execute();
}
