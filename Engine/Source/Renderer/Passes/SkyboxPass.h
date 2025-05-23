#pragma once
#include "../RenderPass.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
class SkyboxPass : public RenderPass
{
public:
	SkyboxPass(const RenderContext& context) : RenderPass(context) {}
	void Execute() override;
};

