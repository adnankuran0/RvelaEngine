#pragma once
#include "../RenderPass.h"
#include "Rendering/Skybox.h"
#include <Rendering/RenderFrame.h>

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