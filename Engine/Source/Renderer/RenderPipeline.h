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

	SkyboxPass* skyboxPass;
	ShadowPass* shadowPass;
	MeshPass* meshPass;


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
};

