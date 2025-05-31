#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
#include <array>


class BrightPass : public RenderPass
{
public:
    BrightPass(const RenderContext& context) : RenderPass(context), screenTexture(-1)
    {
        if (!isInitialized)
        {

            glGenFramebuffers(1, &brightFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);


            glGenTextures(1, &brightColorTex);
            glBindTexture(GL_TEXTURE_2D, brightColorTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, context.viewportWidth, context.viewportHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightColorTex, 0);

            
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "Bright FBO not complete!" << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            isInitialized = true;
        }

    }

    void Execute() override;
    GLuint GetBrightTexture() { return brightColorTex; }

    GLuint screenTexture;

private:
    static bool isInitialized;
    static GLuint brightFBO, brightColorTex;
    static GLuint quadVAO, quadVBO;
};
