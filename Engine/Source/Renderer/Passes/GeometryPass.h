#pragma once
#include "../RenderPass.h"
#include "../Renderer.h"
#include "../../Core/Utils/TextureManager.h"

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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

			glGenTextures(1, &gRoughness);
			glBindTexture(GL_TEXTURE_2D, gRoughness);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gRoughness, 0);

			glGenTextures(1, &gMetallic);
			glBindTexture(GL_TEXTURE_2D, gMetallic);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gMetallic, 0);

			GLuint attachments[3] = {
				GL_COLOR_ATTACHMENT0, // normal
				GL_COLOR_ATTACHMENT1, // roughness
				GL_COLOR_ATTACHMENT2  // metallic
			};
			glDrawBuffers(3, attachments);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "Geometry framebuffer not complete!" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			isInitialized = true;
		}
		
	}
	void Execute() override;

	GLuint GetNormalTexure() { return gNormal; }
	GLuint GetDepthTexure() { return gDepth; }
	GLuint GetMetallicTexure() { return gMetallic; }
	GLuint GetRoughnessTexure() { return gRoughness; }

private:
	static GLuint gBuffer, gNormal, gDepth, gMetallic, gRoughness;
	static bool isInitialized;
};

