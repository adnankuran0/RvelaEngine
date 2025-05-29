#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
#include <array>


class BrightPass : public RenderPass
{
public:
    BrightPass(const RenderContext& context) : RenderPass(context)
    {
        if (!isInitialized)
        {
            //setup quad -- why the fuck is this here?
            float quadVertices[] = {
                // positions   // texCoords
                -1.0f,  1.0f,   0.0f, 1.0f,
                -1.0f, -1.0f,   0.0f, 0.0f,
                 1.0f, -1.0f,   1.0f, 0.0f,

                -1.0f,  1.0f,   0.0f, 1.0f,
                 1.0f, -1.0f,   1.0f, 0.0f,
                 1.0f,  1.0f,   1.0f, 1.0f
            };

            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

            glGenFramebuffers(1, &brightFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);


            glGenTextures(1, &brightColorTex);
            glBindTexture(GL_TEXTURE_2D, brightColorTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, context.viewportWidth, context.viewportHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightColorTex, 0);

            //optional
            GLuint brightRBO;
            glGenRenderbuffers(1, &brightRBO);
            glBindRenderbuffer(GL_RENDERBUFFER, brightRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, context.viewportWidth, context.viewportHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, brightRBO);

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
