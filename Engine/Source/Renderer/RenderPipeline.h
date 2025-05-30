#pragma once
#include <vector>
#include <memory>
#include "RenderPass.h"
#include "Renderer/Passes/MeshPass.h"
#include "Renderer/Passes/SkyboxPass.h"
#include "Renderer/Passes/ShadowPass.h"
#include "Renderer/Passes/PostProcessPass.h"
#include "Renderer/Passes/BrightPass.h"
#include "Renderer/Passes/BloomPass.h"
#include "Renderer/Passes/GeometryPass.h"
#include "Renderer/Passes/SSAOPass.h"

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
	void SetMeshPass(MeshPass* meshPass) { this->meshPass = meshPass; }
	void SetPostProcessPass(PostProcessPass* postProcessPass) { this->postProcessPass = postProcessPass; }
	void SetBrightPass(BrightPass* brightPass) { this->brightPass = brightPass; }
	void SetBloomPass(BloomPass* bloomPass) { this->bloomPass = bloomPass; }
	void SetSSAOPass(SSAOPass* ssaoPass) { this->ssaoPass = ssaoPass; }


	void Execute() noexcept
	{
		skyboxPass->screenFBO = meshPass->GetScreenFBO();
		skyboxPass->Execute();
		shadowPass->Execute();

		geometryPass->Execute();

		ssaoPass->gDepth = geometryPass->GetDepthTexure();
		ssaoPass->gNormal = geometryPass->GetNormalTexure();
		ssaoPass->Execute();

		meshPass->shadowMap = shadowPass->GetDepthMap();
		meshPass->pointShadowMap = shadowPass->GetPointDepthMap();
		meshPass->lightSpaceMatrix = shadowPass->GetLightSpaceMatrix();
		meshPass->Execute();

		brightPass->screenTexture = meshPass->GetScreenTexture();
		brightPass->Execute();

		bloomPass->brightTexture = brightPass->GetBrightTexture();
		bloomPass->Execute();

		postProcessPass->screenTexture = meshPass->GetScreenTexture();
		postProcessPass->bloomBlurTexture = bloomPass->GetBlurTexture();
		postProcessPass->aoTexture = ssaoPass->GetSSAOTexture();
		postProcessPass->Execute();
	}
private:
	std::vector <RenderPass*> passes;
	SkyboxPass* skyboxPass;
	ShadowPass* shadowPass;
	MeshPass* meshPass;
	PostProcessPass* postProcessPass;
	BrightPass* brightPass;
	BloomPass* bloomPass;
	GeometryPass* geometryPass;
	SSAOPass* ssaoPass;
};

