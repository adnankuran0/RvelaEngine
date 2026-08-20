#include "rvelapch.h"
#include "BrightPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

void BrightPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = ctx.viewportWidth / 2;
    desc.height = ctx.viewportHeight / 2;
    desc.colorAttachments = {
        { FramebufferTextureFormat::RGB16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
    };
    desc.hasDepth = false;

    m_Framebuffer = Framebuffer(desc);

    frame.registry.Register("BrightTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

void BrightPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.environment->Bloom) return;

    uint32_t targetWidth = ctx.viewportWidth / 2;
    uint32_t targetHeight = ctx.viewportHeight / 2;
    if (m_Framebuffer.GetWidth() != targetWidth || m_Framebuffer.GetHeight() != targetHeight)
    {
        m_Framebuffer.Resize(targetWidth, targetHeight);
        frame.registry.Register("BrightTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
    }

    auto i_ScreenTexture = frame.registry.Get("ScreenTexture")->id;
    auto& env = *ctx.environment;

    glDisable(GL_DEPTH_TEST);

    Shader& brightShader = ShaderManager::Get("Bright");
    brightShader.use();

    brightShader.setInt("hdrTexture", 0);
    brightShader.setFloat("threshold", env.Bloom_Treshold);
    brightShader.setFloat("knee", env.Bloom_Knee);

    glBindTextureUnit(0, i_ScreenTexture);

    m_Framebuffer.BindViewport();
    glClear(GL_COLOR_BUFFER_BIT);

    Renderer::DrawFullScreenQuad();

    glEnable(GL_DEPTH_TEST);
    Framebuffer::BindDefault();
}