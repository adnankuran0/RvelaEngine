#include "rvelapch.h"
#include "BrightPass.h"
#include "../Renderer.h"

void BrightPass::Init()
{
	glGenFramebuffers(1, &brightFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);


	glGenTextures(1, &o_BrightColorTex);
	glBindTexture(GL_TEXTURE_2D, o_BrightColorTex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R11F_G11F_B10F,           
        ctx.viewportWidth / 2,
        ctx.viewportHeight / 2,
        0,
        GL_RGB,                       
        GL_UNSIGNED_INT_10F_11F_11F_REV,
        nullptr
    );

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_BrightColorTex, 0);


	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		LOG_ERROR("Bright FBO not complete!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

BrightPass::~BrightPass()
{
	//TODO: Fill this funciton
}

void BrightPass::Execute()
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Bright Pass");

    glDisable(GL_DEPTH_TEST);

    Shader& brightShader = Renderer::GetBrightShader();
    brightShader.use();

    brightShader.setInt("hdrTexture", 0);
    brightShader.setFloat("threshold", 1.0f);
    brightShader.setFloat("knee", 0.5f);

    glBindTextureUnit(0, i_ScreenTexture); 

    glBindFramebuffer(GL_FRAMEBUFFER, brightFBO);
	glViewport(0, 0, ctx.viewportWidth / 2, ctx.viewportHeight / 2);
    glClear(GL_COLOR_BUFFER_BIT);

    Renderer::DrawFullScreenQuad();

    glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    glPopDebugGroup();
}