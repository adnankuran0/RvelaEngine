#include "rvelapch.h"
#include "BrightPass.h"
#include "../Renderer.h"

void BrightPass::Init()
{
	glGenFramebuffers(1, &brightFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);


	glGenTextures(1, &o_BrightColorTex);
	glBindTexture(GL_TEXTURE_2D, o_BrightColorTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_BrightColorTex, 0);


	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "Bright FBO not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

BrightPass::~BrightPass()
{
	//TODO: Fill this funciton
}

void BrightPass::Execute()
{

	glDisable(GL_DEPTH_TEST);
	Shader& brightShader = Renderer::GetBrightShader();
	glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	brightShader.use();
	brightShader.setInt("hdrTexture", 0);
	brightShader.setFloat("threshold", 1.0f);
	brightShader.setFloat("knee", 0.5f);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, i_ScreenTexture);
	Renderer::DrawFullScreenQuad();
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}