#include "rvelapch.h"
#include "PostProcessPass.h"
#include "../Renderer.h"

bool PostProcessPass::isInitialized = false;
GLuint PostProcessPass::quadVAO = 0;
GLuint PostProcessPass::quadVBO = 0;
float PostProcessPass::exposure = 1.0f;

void PostProcessPass::Execute()
{
	glDisable(GL_DEPTH_TEST);
	Shader& postProcessShader = Renderer::GetScreenQuadShader();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	UpdateExposure(Time::GetDeltaTime());

	postProcessShader.use();
	//postProcessShader.setFloat("exposure", glm::clamp(exposure, 0.1f, 5.0f)); TODO: take a look at this
	postProcessShader.setFloat("exposure", 1.0f);
	postProcessShader.setInt("screenTexture", 0);
	postProcessShader.setInt("bloomTexture", 1);
	postProcessShader.setInt("aoTexture", 2);
	postProcessShader.setInt("ssrTexture", 3);

	glBindVertexArray(quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bloomBlurTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, aoTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ssrTexture);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glEnable(GL_DEPTH_TEST);

}

void PostProcessPass::UpdateExposure(float deltaTime)
{
	glBindTexture(GL_TEXTURE_2D, screenTexture); // screenTexture
	glGenerateMipmap(GL_TEXTURE_2D);

	int mipLevel = std::log2(std::max(ctx.viewportWidth, ctx.viewportHeight));
	float avgColor[3];
	glGetTexImage(GL_TEXTURE_2D, mipLevel, GL_RGB, GL_FLOAT, avgColor);

	float luminance = 0.2126f * avgColor[0] + 0.7152f * avgColor[1] + 0.0722f * avgColor[2];

	float targetLuminance = 0.5f;
	float key = 0.1f;
	float targetExposure = key * (targetLuminance / (luminance + 0.001f));

	float speed = 1.5f;
	exposure += (targetExposure - exposure) * deltaTime * speed;
}