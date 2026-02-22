#pragma once
#include "../RenderPass.h"
#include "Rendering/Camera.h"
#include <Rendering/RenderFrame.h>

namespace rv {

class LightingPass : public RenderPass
{
public:

    ~LightingPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
	void Execute(const RenderContext& ctx, RenderFrame& frame) override;


private:
	
	GLuint o_ScreenFBO = 0;
	GLuint o_IntermediateColorTex = 0;

	GLuint screenColorTex = 0;
	GLuint screenRBO = 0;
	GLuint intermediateFBO = 0;
};

}