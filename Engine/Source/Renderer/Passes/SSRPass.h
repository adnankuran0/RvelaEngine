#pragma once
#include "../RenderPass.h"

class SSRPass : public RenderPass
{
public:
    SSRPass(const RenderContext& context) : RenderPass(context), gDepth(-1), gNormal(-1), gRoughness(-1), gMetallic(-1), gScreen(-1)
    {
        if (!isInitialized)
        {
            

            glGenFramebuffers(1, &ssrFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);

            glGenTextures(1, &ssrTexture);
            glBindTexture(GL_TEXTURE_2D, ssrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssrTexture, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "SSR framebuffer not complete!" << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            isInitialized = true;
        }
    }

    void Execute() override;

    GLuint gDepth;
    GLuint gNormal;
    GLuint gRoughness;
    GLuint gMetallic;
    GLuint gScreen;

    GLuint GetSSRTexture() const { return ssrTexture; }

private:
    static GLuint ssrFBO;
    static GLuint ssrTexture;
    static bool isInitialized;
};
