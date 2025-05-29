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
	void SetMeshPass(MeshPass* meshPass) { this->meshPass = meshPass; }
	void SetPostProcessPass(PostProcessPass* postProcessPass) { this->postProcessPass = postProcessPass; }
	void SetBrightPass(BrightPass* brightPass) { this->brightPass = brightPass; }
	void SetBloomPass(BloomPass* bloomPass) { this->bloomPass = bloomPass; }


	void Execute() noexcept
	{
		skyboxPass->Execute();
		shadowPass->Execute();
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
};

