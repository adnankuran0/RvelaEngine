#pragma once
#include "../RenderPass.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class SSRPass : public RenderPass
{
public:
    ~SSRPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    
    GLuint o_SsrTexture = 0;
    GLuint ssrFBO = 0;
};

}