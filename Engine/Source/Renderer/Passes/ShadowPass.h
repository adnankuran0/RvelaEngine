#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"

class ShadowPass : public RenderPass
{
public:
    ShadowPass(const RenderContext& context) : RenderPass(context)
    {
        if (!isInitialized)
        {
            glGenFramebuffers(1, &fbo);

            // Higher precision depth texture
            glGenTextures(1, &depthMap);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_DEPTH_COMPONENT32,
                SHADOW_WIDTH,
                SHADOW_HEIGHT,
                0,
                GL_DEPTH_COMPONENT,
                GL_FLOAT,
                nullptr
            );

            // Enable PCF filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            // Attach depth texture to FBO
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "ERROR: Shadow framebuffer not complete!" << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            isInitialized = true;
        }
    }

    void Execute() override;

    GLuint GetDepthMap() const { return depthMap; }
    glm::mat4 GetLightSpaceMatrix() const { return lightSpaceMatrix; }
private:
    static GLuint fbo;
    static GLuint depthMap;
    static glm::mat4 lightSpaceMatrix;
    static bool isInitialized;
    const unsigned int SHADOW_WIDTH = 8192, SHADOW_HEIGHT = 8192;
};

