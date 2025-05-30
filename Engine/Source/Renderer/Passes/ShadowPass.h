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
            // DIRECTIONAL LIGHT SHADOWMAP SETUP
            glGenFramebuffers(1, &fbo);

            glGenTextures(1, &depthMap);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_DEPTH_COMPONENT32F,
                SHADOW_WIDTH,
                SHADOW_HEIGHT,
                0,
                GL_DEPTH_COMPONENT,
                GL_FLOAT,
                nullptr
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "ERROR: Shadow framebuffer not complete!" << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // POINT LIGHT SHADOWMAP SETUP
            glGenFramebuffers(1, &pointFBO);

            glGenTextures(1, &pointDepthMap);
            glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointDepthMap);
            glTexImage3D(
                GL_TEXTURE_CUBE_MAP_ARRAY,
                0,
                GL_DEPTH_COMPONENT32F,
                POINT_SHADOW_WIDTH,
                POINT_SHADOW_HEIGHT,
                6 * 20,
                0,
                GL_DEPTH_COMPONENT,
                GL_FLOAT,
                nullptr
            );

            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
            for (int layer = 0; layer < 6 * 20; ++layer) {
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthMap, 0, layer);
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    std::cerr << "ERROR: Point shadow framebuffer layer " << layer << " not complete!" << std::endl;
                }
            }

            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            isInitialized = true;
        }
    }

    void Execute() override;

    GLuint GetDepthMap() const { return depthMap; }
    GLuint GetPointDepthMap() const { return pointDepthMap; }
    glm::mat4 GetLightSpaceMatrix() const { return lightSpaceMatrix; }
private:
    static GLuint fbo;
    static GLuint depthMap;
    static GLuint pointFBO;
    static GLuint pointDepthMap;
    static glm::mat4 lightSpaceMatrix;
    static bool isInitialized;
    const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
    const unsigned int POINT_SHADOW_WIDTH = 1024, POINT_SHADOW_HEIGHT = 1024;
};

