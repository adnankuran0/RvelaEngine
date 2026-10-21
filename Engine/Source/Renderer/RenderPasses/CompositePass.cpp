#include "rvelapch.h"
#include "CompositePass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/ShaderManager.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Utils/ProjectManager.h"

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
    m_FXAAFramebuffer = Framebuffer(desc);

    frame.registry.Register("CompositeTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
    frame.registry.Register("FinalTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
    frame.registry.Register("FinalFramebuffer", { RenderResourceType::Framebuffer, m_Framebuffer.GetID() });
}

void CompositePass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    if (m_Framebuffer.GetWidth() == width && m_Framebuffer.GetHeight() == height)
        return;

    m_Framebuffer.Resize(width, height);
    m_FXAAFramebuffer.Resize(width, height);

    frame.registry.Register("CompositeTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
    frame.registry.Register("FinalTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
    frame.registry.Register("FinalFramebuffer", { RenderResourceType::Framebuffer, m_Framebuffer.GetID() });
}

void CompositePass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (m_Framebuffer.GetWidth() != ctx.viewportWidth || m_Framebuffer.GetHeight() != ctx.viewportHeight)
    {
        OnResize(ctx.viewportWidth, ctx.viewportHeight, frame);
    }

    auto& env = *ctx.environment;

    auto* mbRes = frame.registry.Get("MotionBlurTexture");
    auto i_ScreenTexture = (mbRes && mbRes->id != 0) ? mbRes->id : frame.registry.Get("ScreenTexture")->id;
    auto i_BloomBlurTexture = frame.registry.Get("BloomTexture")->id;
    auto i_SsrTexture = frame.registry.Get("SSRTexture")->id;

    auto i_DepthTexture = frame.registry.Get("DepthTexture")->id;

    glDisable(GL_DEPTH_TEST);

    m_Framebuffer.BindViewport();
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& compositeShader = ShaderManager::Get("Composite");
    compositeShader.use();
    compositeShader.setFloat("exposure", env.PostProcess_Exposure);
    compositeShader.setFloat("brightness", env.PostProcess_Brightness);
    compositeShader.setFloat("contrast", env.PostProcess_Contrast);
    compositeShader.setFloat("saturation", env.PostProcess_Saturation);
    compositeShader.setFloat("bloomIntensity", env.Bloom ? env.Bloom_Intensity : 0.0f);
    compositeShader.setFloat("vignetteIntensity", env.PostProcess_VignetteIntensity);
    compositeShader.setFloat("vignetteSmoothness", env.PostProcess_VignetteSmoothness);
    compositeShader.setFloat("chromaticStrength", env.PostProcess_ChromaticStrength);

    compositeShader.setBool("u_FogEnabled", env.Fog_Enabled);
    compositeShader.setVec3("u_FogColor", env.Fog_Color);
    compositeShader.setFloat("u_FogDensity", env.Fog_Density);
    compositeShader.setFloat("u_FogStart", env.Fog_Start);
    compositeShader.setFloat("u_FogEnd", env.Fog_End);
    compositeShader.setInt("u_FogMode", env.Fog_Mode);

    compositeShader.setInt("screenTexture", 0);
    compositeShader.setInt("bloomTexture", 1);
    compositeShader.setInt("ssrTexture", 2);
    compositeShader.setInt("depthTexture", 3);

    for (int s = 0; s < 4; ++s)
        glBindSampler(s, 0);

    glBindTextureUnit(0, i_ScreenTexture);
    glBindTextureUnit(1, i_BloomBlurTexture);
    glBindTextureUnit(2, i_SsrTexture);
    glBindTextureUnit(3, i_DepthTexture);

    Renderer::DrawFullScreenQuad();

    for (int s = 0; s < 4; ++s)
        glBindTextureUnit(s, 0);

    bool useFXAA = false;
    if (ProjectManager::IsProjectLoaded())
    {
        useFXAA = ProjectManager::GetActiveProject()->GetSettings().fxaaEnabled;
    }

    if (useFXAA)
    {
        m_FXAAFramebuffer.BindViewport();
        glClear(GL_COLOR_BUFFER_BIT);

        Shader& fxaaShader = ShaderManager::Get("FXAA");
        fxaaShader.use();
        fxaaShader.setInt("u_Texture", 0);
        fxaaShader.setBool("u_FXAAEnabled", true);
        fxaaShader.setVec2("u_TexelSize", glm::vec2(1.0f / ctx.viewportWidth, 1.0f / ctx.viewportHeight));

        glBindTextureUnit(0, m_Framebuffer.GetColorAttachment(0));
        Renderer::DrawFullScreenQuad();

        frame.registry.Register("FinalTexture", { RenderResourceType::Texture, m_FXAAFramebuffer.GetColorAttachment(0) });
        frame.registry.Register("FinalFramebuffer", { RenderResourceType::Framebuffer, m_FXAAFramebuffer.GetID() });
    }
    else
    {
        frame.registry.Register("FinalTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
        frame.registry.Register("FinalFramebuffer", { RenderResourceType::Framebuffer, m_Framebuffer.GetID() });
    }

    GLuint finalTex = useFXAA ? m_FXAAFramebuffer.GetColorAttachment(0) : m_Framebuffer.GetColorAttachment(0);
    GLenum filterMode = GL_NEAREST;
    if (ProjectManager::IsProjectLoaded())
    {
        if (ProjectManager::GetActiveProject()->GetSettings().scalingFilter == ScalingFilter::Linear)
            filterMode = GL_LINEAR;
        else
            filterMode = GL_NEAREST;
    }
    glTextureParameteri(finalTex, GL_TEXTURE_MIN_FILTER, filterMode);
    glTextureParameteri(finalTex, GL_TEXTURE_MAG_FILTER, filterMode);

    glEnable(GL_DEPTH_TEST);
    Framebuffer::BindDefault();
}