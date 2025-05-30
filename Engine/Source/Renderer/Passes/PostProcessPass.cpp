#include "rvelapch.h"
#include "PostProcessPass.h"
#include "../Renderer.h"

bool PostProcessPass::isInitialized = false;
GLuint PostProcessPass::quadVAO = 0;
GLuint PostProcessPass::quadVBO = 0;

void PostProcessPass::Execute()
{
	glDisable(GL_DEPTH_TEST);
	Shader& postProcessShader = Renderer::GetScreenQuadShader();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	postProcessShader.use();
	postProcessShader.setInt("screenTexture", 0);
	postProcessShader.setInt("bloomTexture", 1);
	postProcessShader.setInt("aoTexture", 2);

	glBindVertexArray(quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bloomBlurTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, aoTexture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glEnable(GL_DEPTH_TEST);

}