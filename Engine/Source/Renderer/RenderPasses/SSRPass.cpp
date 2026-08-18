#include "rvelapch.h"
#include "SSRPass.h"
#include "Core/Time.h"
#include "Renderer/Camera.h" 
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

void SSRPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    glGenFramebuffers(1, &ssrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);

    glGenTextures(1, &o_SsrTexture);
    glBindTexture(GL_TEXTURE_2D, o_SsrTexture);

    int w = (int)(ctx.viewportWidth / 2.0f);
    int h = (int)(ctx.viewportHeight / 2.0f);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_SsrTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LOG_WARN("SSR framebuffer not complete!");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("SSRTexture", { RenderResourceType::Texture, o_SsrTexture });
}

SSRPass::~SSRPass()
{
    glDeleteTextures(1, &o_SsrTexture);
    glDeleteFramebuffers(1, &ssrFBO);
}

void SSRPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.environment->SSR) return;

    auto i_Depth = frame.registry.Get("DepthTexture")->id;
    auto i_Normal = frame.registry.Get("NormalTexture")->id;
    auto i_Roughness = frame.registry.Get("RoughnessTexture")->id;
    auto i_Metallic = frame.registry.Get("MetallicTexture")->id;
    auto i_Screen = frame.registry.Get("ScreenTexture")->id;
    auto i_Skybox = frame.registry.Get("SkyboxTexture")->id;

    Shader& ssrShader = ShaderManager::Get("SSR");
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);
    glViewport(0, 0, ctx.viewportWidth / 2, ctx.viewportHeight / 2);
    glClear(GL_COLOR_BUFFER_BIT);

    ssrShader.use();

    glBindTextureUnit(0, i_Depth);
    glBindTextureUnit(1, i_Normal);
    glBindTextureUnit(2, i_Roughness);
    glBindTextureUnit(3, i_Metallic);
    glBindTextureUnit(4, i_Screen);
    glBindTextureUnit(5, i_Skybox);

    ssrShader.setInt("uDepthTexture", 0);
    ssrShader.setInt("uNormalTexture", 1);
    ssrShader.setInt("uRoughnessTexture", 2);
    ssrShader.setInt("uMetallicTexture", 3);
    ssrShader.setInt("uScreenTexture", 4);
    ssrShader.setInt("uSkybox", 5);

    Renderer::DrawFullScreenQuad();

    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    frame.registry.Register("SSRTexture", { RenderResourceType::Texture, o_SsrTexture });
}