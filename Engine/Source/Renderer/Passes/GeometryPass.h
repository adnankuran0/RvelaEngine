#pragma once
#include "../RenderPass.h"
#include "../Renderer.h"

class GeometryPass : public RenderPass
{
public:
	GeometryPass(const RenderContext& context) : RenderPass(context)
	{
		if (!isInitialized)
		{
			glGenFramebuffers(1, &gBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

			glGenTextures(1, &gNormal);
			glBindTexture(GL_TEXTURE_2D, gNormal);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RGB, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gNormal, 0);

			glGenTextures(1, &gDepth);
			glBindTexture(GL_TEXTURE_2D, gDepth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, ctx.viewportWidth, ctx.viewportHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

			GLuint attachments[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, attachments);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "Geometry framebuffer not complete!" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			isInitialized = true;
		}
		
	}
	void Execute() override;

	GLuint GetNormalTexure() { return gNormal; }
	GLuint GetDepthTexure() { return gDepth; }

private:
	static GLuint gBuffer, gNormal, gDepth;
	static bool isInitialized;
};

