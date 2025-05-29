#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"


class MeshPass : public RenderPass
{
public:
	MeshPass(const RenderContext& context) : RenderPass(context) 
	{
		if (!isInitialized)
		{
			glGenFramebuffers(1, &screenFBO);
			glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
			// Color Attachment (texture)
			glGenTextures(1, &screenColorTex);
			glBindTexture(GL_TEXTURE_2D, screenColorTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, context.viewportWidth, context.viewportHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenColorTex, 0);

			// Depth/Stencil Attachment (Renderbuffer)
			glGenRenderbuffers(1, &screenRBO);
			glBindRenderbuffer(GL_RENDERBUFFER, screenRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, context.viewportWidth, context.viewportHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screenRBO);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cerr << "Framebuffer not complete" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			isInitialized = true;
		}
		
	}

	void Execute() override;

	GLuint GetScreenTexture() { return screenColorTex; }

	GLuint shadowMap = 0;
	GLuint pointShadowMap = 0;
	glm::mat4 lightSpaceMatrix;

private:
	static GLuint screenFBO, screenColorTex, screenRBO;
	static bool isInitialized;
};
