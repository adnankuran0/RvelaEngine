#pragma once
#include "../RenderPass.h"
#include "Renderer/Framebuffer.h"
#include <Renderer/RenderFrame.h>
namespace rv {
class SSAOPass : public RenderPass
{
public:
    ~SSAOPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;
private:
    Framebuffer m_Framebuffer;
    GLuint noiseTexture = 0;
    void GenerateNoiseTexture();
};
}
