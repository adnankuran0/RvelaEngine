#include "rvelapch.h"
#include "CompositePass.h"
#include "../Renderer.h"

void CompositePass::Execute()
{
	glDisable(GL_DEPTH_TEST);
	Shader& compositeShader = Renderer::GetCompositeShader();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT);


	compositeShader.use();
	//UpdateExposure(Time::GetDeltaTime());
	//postProcessShader.setFloat("exposure", glm::clamp(exposure, 0.1f, 5.0f)); TODO: take a look at this
	compositeShader.setFloat("exposure", 1.0f);
	compositeShader.setInt("screenTexture", 0);
	compositeShader.setInt("bloomTexture", 1);
	compositeShader.setInt("aoTexture", 2);
	compositeShader.setInt("ssrTexture", 3);

	glBindTextureUnit(0, i_ScreenTexture);
	glBindTextureUnit(1, i_BloomBlurTexture);
	glBindTextureUnit(2, i_AoTexture);
	glBindTextureUnit(3, i_SsrTexture);

	Renderer::DrawFullScreenQuad();
	glEnable(GL_DEPTH_TEST);

}

void CompositePass::UpdateExposure(float deltaTime)
{
	glBindTexture(GL_TEXTURE_2D, i_ScreenTexture); // screenTexture
	glGenerateMipmap(GL_TEXTURE_2D);

	int mipLevel = (int)std::log2(std::max(ctx.viewportWidth, ctx.viewportHeight));
	float avgColor[3];
	glGetTexImage(GL_TEXTURE_2D, mipLevel, GL_RGB, GL_FLOAT, avgColor);

	float luminance = 0.2126f * avgColor[0] + 0.7152f * avgColor[1] + 0.0722f * avgColor[2];

	float targetLuminance = 0.5f;
	float key = 0.1f;
	float targetExposure = key * (targetLuminance / (luminance + 0.001f));

	float speed = 1.5f;
	exposure += (targetExposure - exposure) * deltaTime * speed;
}