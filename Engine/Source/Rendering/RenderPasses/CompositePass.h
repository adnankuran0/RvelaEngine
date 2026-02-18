#pragma once
#include "../RenderPass.h"
#include <Rendering/RenderFrame.h>

namespace rv {

class CompositePass : public RenderPass
{
public:

    ~CompositePass() {}
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
	void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    GLuint m_LuminanceBuffer = 0;
    GLuint m_Framebuffer = 0;       
    GLuint o_FinalTexture = 0;      
};

}