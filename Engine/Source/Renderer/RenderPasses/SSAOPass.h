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
private:
    Framebuffer m_Framebuffer;
    GLuint noiseTexture = 0;
    std::vector<glm::vec3> kernel;
    void GenerateSampleKernel();
    void GenerateNoiseTexture();
};
}