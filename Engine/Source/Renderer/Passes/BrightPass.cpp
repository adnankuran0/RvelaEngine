#include "rvelapch.h"
#include "BrightPass.h"
#include "../Renderer.h"

bool BrightPass::isInitialized = false;
GLuint BrightPass::brightFBO = 0;
GLuint BrightPass::brightColorTex = 0;
GLuint BrightPass::quadVAO = 0;
GLuint BrightPass::quadVBO = 0;

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
	glBindVertexArray(quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}