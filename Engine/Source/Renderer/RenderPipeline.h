#pragma once
#include <vector>
#include <memory>
#include "RenderPass.h"
#include "Renderer/Passes/MeshPass.h"
#include "Renderer/Passes/SkyboxPass.h"
#include "Renderer/Passes/ShadowPass.h"

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


	void Execute() noexcept
	{
		skyboxPass->Execute();
		shadowPass->Execute();
		meshPass->shadowMap = shadowPass->GetDepthMap();
		meshPass->lightSpaceMatrix = shadowPass->GetLightSpaceMatrix();
		meshPass->Execute();
	}
private:
	std::vector <RenderPass*> passes;
	SkyboxPass* skyboxPass;
	ShadowPass* shadowPass;
	MeshPass* meshPass;
};

