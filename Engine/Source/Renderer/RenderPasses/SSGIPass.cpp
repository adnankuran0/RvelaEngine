#include "rvelapch.h"
#include "SSGIPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

void SSGIPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = std::max(1u, ctx.viewportWidth / 2);
    desc.height = std::max(1u, ctx.viewportHeight / 2);
    desc.colorAttachments = {
        { FramebufferTextureFormat::RGB16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
    };
    desc.hasDepth = false;

    m_SSGIFBO = Framebuffer(desc);

    frame.registry.Register("SSGITexture", { RenderResourceType::Texture, m_SSGIFBO.GetColorAttachment(0) });
}

SSGIPass::~SSGIPass()
{
}

void SSGIPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.environment->SSGI) return;

    auto& env = *ctx.environment;

    auto i_Depth = frame.registry.Get("DepthTexture")->id;
    auto i_Normal = frame.registry.Get("NormalTexture")->id;
    auto i_Screen = frame.registry.Get("ScreenTexture")->id;
    auto screenFBO = frame.registry.Get("ScreenBuffer")->id;
    auto intermediateFBO = frame.registry.Get("IntermediateBuffer")->id;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glBlitFramebuffer(
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    // Raymarch at half resolution
    m_SSGIFBO.BindViewport();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    Shader& ssgiShader = ShaderManager::Get("SSGI");
    ssgiShader.use();

    glBindTextureUnit(0, i_Depth);
    glBindTextureUnit(1, i_Normal);
    glBindTextureUnit(2, i_Screen);

    ssgiShader.setInt("gDepth", 0);
    ssgiShader.setInt("gNormal", 1);
    ssgiShader.setInt("uScreenTexture", 2);

    ssgiShader.setFloat("u_Radius", env.SSGI_Radius);
    ssgiShader.setFloat("u_Intensity", env.SSGI_Intensity);
    ssgiShader.setInt("u_Steps", env.SSGI_Steps);
    ssgiShader.setFloat("u_Thickness", 0.5f);

    Renderer::DrawFullScreenQuad();

    // Blur and additive blend back to ScreenBuffer
    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE); // Add indirect diffuse radiance
    glBlendEquation(GL_FUNC_ADD);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    Shader& blendShader = ShaderManager::Get("SSGIBlend");
    blendShader.use();

    glBindTextureUnit(0, m_SSGIFBO.GetColorAttachment(0));
    glBindTextureUnit(1, i_Depth);
    glBindTextureUnit(2, i_Normal);

    blendShader.setInt("u_SSGITexture", 0);
    blendShader.setInt("gDepth", 1);
    blendShader.setInt("gNormal", 2);
    blendShader.setVec2("u_TexelSize", glm::vec2(1.0f / (ctx.viewportWidth / 2.0f), 1.0f / (ctx.viewportHeight / 2.0f)));

    Renderer::DrawFullScreenQuad();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}
