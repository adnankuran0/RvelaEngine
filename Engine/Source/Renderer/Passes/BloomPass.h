#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
#include <array>
#include <algorithm>

class BloomPass : public RenderPass
{
public:
    BloomPass(const RenderContext& context) : RenderPass(context)
    {
        if (!isInitialized)
        {
            float quadVertices[] = {
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

            downsampleFBOs.resize(mipLevels);
            downsampleTextures.resize(mipLevels);
            upsampleFBOs.resize(mipLevels);
            upsampleTextures.resize(mipLevels);

            for (int i = 0; i < mipLevels; ++i)
            {
                int w = std::max((unsigned int)1, context.viewportWidth >> i);
                int h = std::max((unsigned int)1, context.viewportHeight >> i);

                GLuint texDown, fboDown, texUp, fboUp;

                glGenTextures(1, &texDown);
                glBindTexture(GL_TEXTURE_2D, texDown);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glGenFramebuffers(1, &fboDown);
                glBindFramebuffer(GL_FRAMEBUFFER, fboDown);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texDown, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, fboDown);
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                {
                    std::cerr << "Downsample framebuffer incomplete at mip level " << i << std::endl;
                }

                glGenTextures(1, &texUp);
                glBindTexture(GL_TEXTURE_2D, texUp);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                glGenFramebuffers(1, &fboUp);
                glBindFramebuffer(GL_FRAMEBUFFER, fboUp);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texUp, 0);

                downsampleTextures[i] = texDown;
                downsampleFBOs[i] = fboDown;
                upsampleTextures[i] = texUp;
                upsampleFBOs[i] = fboUp;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            isInitialized = true;
        }
    }
    void Execute() override;
    GLuint GetBlurTexture() { return blurredTexture; }

    GLuint brightTexture;

private:
    static bool isInitialized;
    static GLuint quadVAO, quadVBO;
    static std::vector<GLuint> downsampleFBOs;
    static std::vector<GLuint> downsampleTextures;

    static std::vector<GLuint> upsampleFBOs;
    static std::vector<GLuint> upsampleTextures;

    static GLuint blurredTexture;
    static constexpr int mipLevels = 6;
};
