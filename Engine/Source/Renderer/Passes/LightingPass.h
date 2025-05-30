#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"


class LightingPass : public RenderPass
{
public:
    LightingPass(const RenderContext& context) : RenderPass(context)
	{
        if (!isInitialized)
        {
            // MSAA framebuffer
            glGenFramebuffers(1, &screenFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);

            glGenTextures(1, &screenColorTex);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, screenColorTex);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA16F, context.viewportWidth, context.viewportHeight, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, screenColorTex, 0);

            glGenRenderbuffers(1, &screenRBO);
            glBindRenderbuffer(GL_RENDERBUFFER, screenRBO);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, context.viewportWidth, context.viewportHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screenRBO);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "MSAA Framebuffer not complete" << std::endl;

            // Intermediate framebuffer (non-MSAA)
            glGenFramebuffers(1, &intermediateFBO);
            glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

            glGenTextures(1, &intermediateColorTex);
            glBindTexture(GL_TEXTURE_2D, intermediateColorTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, context.viewportWidth, context.viewportHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, intermediateColorTex, 0);

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cerr << "Intermediate Framebuffer not complete" << std::endl;

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            isInitialized = true;
        }
		
	}

	void Execute() override;

	GLuint GetScreenTexture() { return intermediateColorTex; }
    GLuint GetScreenFBO() { return screenFBO; }

	GLuint shadowMap = 0;
	GLuint pointShadowMap = 0;
	glm::mat4 lightSpaceMatrix;

private:
	static GLuint screenFBO, screenColorTex, screenRBO;
	static GLuint intermediateFBO, intermediateColorTex;
	static bool isInitialized;
};
