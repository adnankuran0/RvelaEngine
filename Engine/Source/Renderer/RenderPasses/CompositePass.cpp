#include "rvelapch.h"
#include "CompositePass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/ShaderManager.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"

using namespace rv;

void CompositePass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = ctx.viewportWidth;
    desc.height = ctx.viewportHeight;
    desc.hasDepth = false;
    desc.colorAttachments = {
        { FramebufferTextureFormat::RGBA16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
    };

    m_Framebuffer = Framebuffer(desc);

    frame.registry.Register("FinalTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
    frame.registry.Register("FinalFramebuffer", { RenderResourceType::Framebuffer, m_Framebuffer.GetID() });
}

void CompositePass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (m_Framebuffer.GetWidth() != ctx.viewportWidth || m_Framebuffer.GetHeight() != ctx.viewportHeight)
    {
        m_Framebuffer.Resize(ctx.viewportWidth, ctx.viewportHeight);
        frame.registry.Register("FinalTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
        frame.registry.Register("FinalFramebuffer", { RenderResourceType::Framebuffer, m_Framebuffer.GetID() });
    }

    auto& env = *ctx.environment;

    auto i_ScreenTexture = frame.registry.Get("ScreenTexture")->id;
    auto i_BloomBlurTexture = frame.registry.Get("BloomTexture")->id;
    auto i_SsrTexture = frame.registry.Get("SSRTexture")->id;

    glDisable(GL_DEPTH_TEST);

    m_Framebuffer.BindViewport();
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& compositeShader = ShaderManager::Get("Composite");
    compositeShader.use();
    compositeShader.setFloat("exposure", env.PostProcess_Exposure);
    compositeShader.setFloat("bloomIntensity", env.Bloom_Intensity);
    compositeShader.setFloat("vignetteIntensity", env.PostProcess_VignetteIntensity);
    compositeShader.setFloat("vignetteSmoothness", env.PostProcess_VignetteSmoothness);
    compositeShader.setFloat("chromaticStrength", env.PostProcess_ChromaticStrength);

    compositeShader.setInt("screenTexture", 0);
    compositeShader.setInt("bloomTexture", 1);
    compositeShader.setInt("ssrTexture", 2);

    glBindTextureUnit(0, i_ScreenTexture);
    glBindTextureUnit(1, i_BloomBlurTexture);
    glBindTextureUnit(2, i_SsrTexture);

    Renderer::DrawFullScreenQuad();

    glEnable(GL_DEPTH_TEST);
    Framebuffer::BindDefault();
}