#pragma once
#include "../RenderPass.h"

class SSAOPass : public RenderPass
{
public:
    SSAOPass(const RenderContext& context) : RenderPass(context), gNormal(-1), gDepth(-1)
    {
        if (!isInitialized)
        {
            

            glGenFramebuffers(1, &ssaoFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

            glGenTextures(1, &ssaoTexture);
            glBindTexture(GL_TEXTURE_2D, ssaoTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTexture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "SSAO framebuffer not complete!" << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            GenerateSampleKernel();
            GenerateNoiseTexture();

            isInitialized = true;
        }
    }
    void Execute() override;

    GLuint gNormal;
    GLuint gDepth;

    GLuint GetSSAOTexture() const { return ssaoTexture; }

private:
    static GLuint ssaoFBO;
    static GLuint ssaoTexture;
    static GLuint noiseTexture;
    static glm::vec3 kernel[64];
    static bool isInitialized;

    void GenerateSampleKernel();
    void GenerateNoiseTexture();

};