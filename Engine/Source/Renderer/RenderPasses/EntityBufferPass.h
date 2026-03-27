#pragma once
#include "../RenderPass.h"

namespace rv {

class EntityBufferPass : public RenderPass
{
public:

    ~EntityBufferPass() {}
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    GLuint m_Framebuffer = 0;
    GLuint m_Renderbuffer = 0;
    GLuint o_EntityTexture = 0;
 
};

}