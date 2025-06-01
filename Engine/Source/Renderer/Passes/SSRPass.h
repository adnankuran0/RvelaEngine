#pragma once
#include "../RenderPass.h"

class SSRPass : public RenderPass
{
public:
    ~SSRPass();
    void Execute() override;
    void Init() override;

    void SetDepthTexture(GLuint depthTexture) { i_Depth = depthTexture; }
    void SetNormalTexture(GLuint normalTexture) { i_Normal = normalTexture; }
    void SetRoughnessTexture(GLuint roughnessTexture) { i_Roughness = roughnessTexture; }
    void SetMetallicTexture(GLuint metallicTexture) { i_Metallic = metallicTexture; }
    void SetScreenTexture(GLuint screenTexture) { i_Screen = screenTexture; }

    GLuint GetSSRTexture() const { return o_SsrTexture; }

private:
    GLuint i_Depth = 0;
    GLuint i_Normal = 0;
    GLuint i_Roughness = 0;
    GLuint i_Metallic = 0;
    GLuint i_Screen = 0;
    GLuint o_SsrTexture = 0;

    GLuint ssrFBO = 0;
};
