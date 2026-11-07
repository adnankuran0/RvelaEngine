#include "rvelapch.h"
#include "SSAOPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

void SSAOPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    m_HalfResolution = ctx.environment ? ctx.environment->SSAO_HalfResolution : true;
    uint32_t targetWidth = m_HalfResolution ? std::max(1u, ctx.viewportWidth / 2) : std::max(1u, ctx.viewportWidth);
    uint32_t targetHeight = m_HalfResolution ? std::max(1u, ctx.viewportHeight / 2) : std::max(1u, ctx.viewportHeight);

    FramebufferDesc desc;
    desc.width = targetWidth;
    desc.height = targetHeight;
    desc.colorAttachments = {
        { FramebufferTextureFormat::R16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
    };
    desc.hasDepth = false;

    m_RawFramebuffer = Framebuffer(desc);
    m_BlurFramebuffer = Framebuffer(desc);

    frame.registry.Register("SSAOTexture", { RenderResourceType::Texture, m_BlurFramebuffer.GetColorAttachment(0) });
    frame.registry.Register("RawSSAOTexture", { RenderResourceType::Texture, m_RawFramebuffer.GetColorAttachment(0) });
}

void SSAOPass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    uint32_t targetWidth = m_HalfResolution ? std::max(1u, width / 2) : std::max(1u, width);
    uint32_t targetHeight = m_HalfResolution ? std::max(1u, height / 2) : std::max(1u, height);

    if (m_BlurFramebuffer.GetWidth() == targetWidth && m_BlurFramebuffer.GetHeight() == targetHeight)
        return;

    m_RawFramebuffer.Resize(targetWidth, targetHeight);
    m_BlurFramebuffer.Resize(targetWidth, targetHeight);

    frame.registry.Register("SSAOTexture", { RenderResourceType::Texture, m_BlurFramebuffer.GetColorAttachment(0) });
    frame.registry.Register("RawSSAOTexture", { RenderResourceType::Texture, m_RawFramebuffer.GetColorAttachment(0) });
}

void SSAOPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.environment || !ctx.environment->SSAO) return;

    bool halfRes = ctx.environment->SSAO_HalfResolution;
    if (halfRes != m_HalfResolution)
    {
        m_HalfResolution = halfRes;
    }
    uint32_t targetWidth = m_HalfResolution ? std::max(1u, ctx.viewportWidth / 2) : std::max(1u, ctx.viewportWidth);
    uint32_t targetHeight = m_HalfResolution ? std::max(1u, ctx.viewportHeight / 2) : std::max(1u, ctx.viewportHeight);

    if (m_BlurFramebuffer.GetWidth() != targetWidth || m_BlurFramebuffer.GetHeight() != targetHeight)
    {
        m_RawFramebuffer.Resize(targetWidth, targetHeight);
        m_BlurFramebuffer.Resize(targetWidth, targetHeight);
        frame.registry.Register("SSAOTexture", { RenderResourceType::Texture, m_BlurFramebuffer.GetColorAttachment(0) });
        frame.registry.Register("RawSSAOTexture", { RenderResourceType::Texture, m_RawFramebuffer.GetColorAttachment(0) });
    }

    auto* normalRes = frame.registry.Get("NormalTexture");
    auto* depthRes = frame.registry.Get("DepthTexture");
    if (!normalRes || !depthRes || normalRes->id == 0 || depthRes->id == 0)
        return;

    auto i_Normal = normalRes->id;
    auto i_Depth = depthRes->id;
    auto& env = *ctx.environment;

    m_RawFramebuffer.BindViewport();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& ssaoShader = ShaderManager::Get("SSAO");
    ssaoShader.use();
    ssaoShader.setFloat("radius", env.SSAO_Radius);
    ssaoShader.setFloat("bias", env.SSAO_Bias);
    ssaoShader.setFloat("intensity", env.SSAO_Intensity);
    ssaoShader.setVec2("u_TargetResolution", glm::vec2(static_cast<float>(targetWidth), static_cast<float>(targetHeight)));

    glBindTextureUnit(0, i_Normal);
    glBindTextureUnit(1, i_Depth);

    ssaoShader.setInt("gNormal", 0);
    ssaoShader.setInt("gDepth", 1);

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);

    m_BlurFramebuffer.BindViewport();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& blurShader = ShaderManager::Get("SSAOBlur");
    blurShader.use();
    blurShader.setInt("u_RawSSAO", 0);
    blurShader.setInt("gDepth", 1);
    blurShader.setInt("gNormal", 2);
    blurShader.setVec2("u_TexelSize", glm::vec2(1.0f / static_cast<float>(targetWidth), 1.0f / static_cast<float>(targetHeight)));

    glBindTextureUnit(0, m_RawFramebuffer.GetColorAttachment(0));
    glBindTextureUnit(1, i_Depth);
    glBindTextureUnit(2, i_Normal);

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);

    Framebuffer::BindDefault();
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}
