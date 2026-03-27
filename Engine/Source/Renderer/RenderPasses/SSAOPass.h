#pragma once
#include "../RenderPass.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class SSAOPass : public RenderPass
{
public:
    ~SSAOPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    GLuint o_SsaoTexture = 0;
    GLuint ssaoFBO = 0;
    GLuint noiseTexture = 0;
    std::vector<glm::vec3> kernel[64];

    void GenerateSampleKernel();
    void GenerateNoiseTexture();

};

}