#include "rvelapch.h"
#include "CompositePass.h"
#include "Rendering/RenderContext.h"

using namespace rv;


void CompositePass::Init(const RenderContext& ctx, RenderFrame& frame)
{
	glGenFramebuffers(1, &m_Framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

	glGenTextures(1, &o_FinalTexture);
	glBindTexture(GL_TEXTURE_2D, o_FinalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ctx.viewportWidth, ctx.viewportHeight,
		0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_FinalTexture, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	frame.registry.Register("FinalTexture", { RenderResourceType::Texture,o_FinalTexture });
	frame.registry.Register("FinalFramebuffer",
		{ RenderResourceType::Framebuffer, m_Framebuffer });
}

void CompositePass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
	auto i_ScreenTexture = frame.registry.Get("ScreenTexture")->id;
	auto i_BloomBlurTexture = frame.registry.Get("BloomTexture")->id;
	auto i_AoTexture = frame.registry.Get("SSAOTexture")->id;
	auto i_SsrTexture = frame.registry.Get("SSRTexture")->id;

	glDisable(GL_DEPTH_TEST);
	Shader& compositeShader = ShaderManager::Get("Composite");
	glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
	glClear(GL_COLOR_BUFFER_BIT);


	compositeShader.use();
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
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);

	frame.registry.Register("FinalTexture", { RenderResourceType::Texture,o_FinalTexture });
	frame.registry.Register("FinalFramebuffer",
		{ RenderResourceType::Framebuffer, m_Framebuffer });
}

