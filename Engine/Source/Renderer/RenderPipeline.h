#pragma once
#include <vector>
#include <memory>
#include "RenderPass.h"
#include "Renderer/Passes/LightingPass.h"
#include "Renderer/Passes/SkyboxPass.h"
#include "Renderer/Passes/ShadowPass.h"
#include "Renderer/Passes/PostProcessPass.h"
#include "Renderer/Passes/BrightPass.h"
#include "Renderer/Passes/BloomPass.h"
#include "Renderer/Passes/GeometryPass.h"
#include "Renderer/Passes/SSAOPass.h"
#include "Renderer/Passes/SSRPass.h"

class RenderPipeline
{
public:
	inline void Clear() noexcept
	{
		passes.clear();
	}

	inline void AddPass(RenderPass* pass) noexcept
	{
		passes.push_back(pass);
	}

	void SetSkyboxPass(SkyboxPass* skyboxPass) { this->skyboxPass = skyboxPass; }
	void SetShadowPass(ShadowPass* shadowPass) { this->shadowPass = shadowPass; }
	void SetGeometryPass(GeometryPass* geometryPass) { this->geometryPass = geometryPass; }
	void SetLightingPass(LightingPass* lightingPass) { this->lightingPass = lightingPass; }
	void SetPostProcessPass(PostProcessPass* postProcessPass) { this->postProcessPass = postProcessPass; }
	void SetBrightPass(BrightPass* brightPass) { this->brightPass = brightPass; }
	void SetBloomPass(BloomPass* bloomPass) { this->bloomPass = bloomPass; }
	void SetSSAOPass(SSAOPass* ssaoPass) { this->ssaoPass = ssaoPass; }
	void SetSSRPass(SSRPass* ssrPass) { this->ssrPass = ssrPass; }


	void Execute() noexcept
	{
		skyboxPass->screenFBO = lightingPass->GetScreenFBO();
		skyboxPass->Execute();
		shadowPass->Execute();

		geometryPass->Execute();

		ssaoPass->gDepth = geometryPass->GetDepthTexure();
		ssaoPass->gNormal = geometryPass->GetNormalTexure();
		ssaoPass->Execute();

		lightingPass->shadowMap = shadowPass->GetDepthMap();
		lightingPass->pointShadowMap = shadowPass->GetPointDepthMap();
		lightingPass->lightSpaceMatrix = shadowPass->GetLightSpaceMatrix();
		lightingPass->Execute();

		brightPass->screenTexture = lightingPass->GetScreenTexture();
		brightPass->Execute();

		bloomPass->brightTexture = brightPass->GetBrightTexture();
		bloomPass->Execute();

		ssrPass->gDepth = geometryPass->GetDepthTexure();
		ssrPass->gNormal = geometryPass->GetNormalTexure();
		ssrPass->gRoughness = geometryPass->GetRoughnessTexure();
		ssrPass->gMetallic = geometryPass->GetMetallicTexure();
		ssrPass->gScreen = lightingPass->GetScreenTexture();
		ssrPass->Execute();

		postProcessPass->screenTexture = lightingPass->GetScreenTexture();
		postProcessPass->bloomBlurTexture = bloomPass->GetBlurTexture();
		postProcessPass->aoTexture = ssaoPass->GetSSAOTexture();
		postProcessPass->ssrTexture = ssrPass->GetSSRTexture();
		postProcessPass->Execute();
	}
private:
	std::vector <RenderPass*> passes;
	SkyboxPass* skyboxPass;
	ShadowPass* shadowPass;
	LightingPass* lightingPass;
	PostProcessPass* postProcessPass;
	BrightPass* brightPass;
	BloomPass* bloomPass;
	GeometryPass* geometryPass;
	SSAOPass* ssaoPass;
	SSRPass* ssrPass;
};

