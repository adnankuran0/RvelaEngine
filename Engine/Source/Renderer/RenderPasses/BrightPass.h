#pragma once
#include "../RenderPass.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class BrightPass : public RenderPass
{
public:
    ~BrightPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    GLuint o_BrightColorTex = 0;
    GLuint brightFBO = 0;
};

}