#pragma once
#include "../RenderPass.h"

class SSAOPass : public RenderPass
{
public:
    ~SSAOPass();
    void Execute() override;
    void Init() override;

    GLuint GetSSAOTexture() const { return o_SsaoTexture; }
    void SetDepthTexture(GLuint depthTexture) { i_Depth = depthTexture; }
    void SetNormalTexture(GLuint normalTexture) { i_Normal = normalTexture; }

private:
    GLuint i_Normal = 0;
    GLuint i_Depth = 0;
    GLuint o_SsaoTexture = 0;
    GLuint ssaoFBO = 0;
    GLuint noiseTexture = 0;
    glm::vec3 kernel[64];

    void GenerateSampleKernel();
    void GenerateNoiseTexture();

};