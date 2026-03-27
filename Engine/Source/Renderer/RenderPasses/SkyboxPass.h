#pragma once
#include "../RenderPass.h"
#include "Renderer/Skybox.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class SkyboxPass : public RenderPass
{
public:
	~SkyboxPass();
	void Execute(const RenderContext& ctx, RenderFrame& frame) override;
	void Init(const RenderContext& ctx, RenderFrame& frame) override;
private:
};

}