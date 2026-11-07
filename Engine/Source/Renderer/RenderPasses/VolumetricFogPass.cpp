#include "rvelapch.h"
#include "VolumetricFogPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"
#include "Scene/Environment.h"

using namespace rv;

void VolumetricFogPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    m_HalfResolution = ctx.environment ? ctx.environment->VolumetricFog_HalfResolution : true;
    uint32_t w = m_HalfResolution ? std::max(1u, ctx.viewportWidth / 2) : std::max(1u, ctx.viewportWidth);
    uint32_t h = m_HalfResolution ? std::max(1u, ctx.viewportHeight / 2) : std::max(1u, ctx.viewportHeight);

    FramebufferDesc desc;
    desc.width = w;
    desc.height = h;
    desc.colorAttachments = {
        { FramebufferTextureFormat::RGBA16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
    };
    desc.hasDepth = false;

    m_FogFramebuffer = Framebuffer(desc);
    frame.registry.Register("VolumetricFogTexture", { RenderResourceType::Texture, m_FogFramebuffer.GetColorAttachment(0) });
}

void VolumetricFogPass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    uint32_t w = m_HalfResolution ? std::max(1u, width / 2) : std::max(1u, width);
    uint32_t h = m_HalfResolution ? std::max(1u, height / 2) : std::max(1u, height);

    if (m_FogFramebuffer.GetWidth() == w && m_FogFramebuffer.GetHeight() == h)
        return;

    m_FogFramebuffer.Resize(w, h);
    frame.registry.Register("VolumetricFogTexture", { RenderResourceType::Texture, m_FogFramebuffer.GetColorAttachment(0) });
}

void VolumetricFogPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.environment || !ctx.environment->VolumetricFog)
        return;

    bool halfRes = ctx.environment->VolumetricFog_HalfResolution;
    if (halfRes != m_HalfResolution)
    {
        m_HalfResolution = halfRes;
    }

    uint32_t targetWidth = m_HalfResolution ? std::max(1u, ctx.viewportWidth / 2) : std::max(1u, ctx.viewportWidth);
    uint32_t targetHeight = m_HalfResolution ? std::max(1u, ctx.viewportHeight / 2) : std::max(1u, ctx.viewportHeight);

    if (m_FogFramebuffer.GetWidth() != targetWidth || m_FogFramebuffer.GetHeight() != targetHeight)
    {
        m_FogFramebuffer.Resize(targetWidth, targetHeight);
        frame.registry.Register("VolumetricFogTexture", { RenderResourceType::Texture, m_FogFramebuffer.GetColorAttachment(0) });
    }

    auto* depthRes = frame.registry.Get("DepthTexture");
    auto* shadowRes = frame.registry.Get("DirectionalShadowMap");
    auto* screenBufferRes = frame.registry.Get("ScreenBuffer");
    auto* intermediateBufferRes = frame.registry.Get("IntermediateBuffer");

    if (!depthRes || !screenBufferRes || !intermediateBufferRes)
        return;

    GLuint i_Depth = depthRes->id;
    GLuint i_ShadowMap = shadowRes ? shadowRes->id : 0;
    GLuint screenFBO = screenBufferRes->id;
    GLuint intermediateFBO = intermediateBufferRes->id;

    auto& env = *ctx.environment;

    m_FogFramebuffer.BindViewport();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& fogShader = ShaderManager::Get("VolumetricFog");
    fogShader.use();

    glBindTextureUnit(0, i_Depth);
    if (i_ShadowMap != 0)
        glBindTextureUnit(1, i_ShadowMap);

    fogShader.setInt("gDepth", 0);
    fogShader.setInt("u_DirectionalShadowMap", 1);

    fogShader.setFloat("u_FogDensity", env.VolumetricFog_Density);
    fogShader.setFloat("u_Extinction", env.VolumetricFog_Extinction);
    fogShader.setFloat("u_Anisotropy", env.VolumetricFog_Anisotropy);
    fogShader.setFloat("u_MaxDistance", env.VolumetricFog_MaxDistance);
    fogShader.setFloat("u_HeightFalloff", env.VolumetricFog_HeightFalloff);
    fogShader.setFloat("u_BaseHeight", env.VolumetricFog_BaseHeight);
    fogShader.setFloat("u_Intensity", env.VolumetricFog_Intensity);
    fogShader.setFloat("u_AmbientIntensity", env.VolumetricFog_AmbientIntensity);
    fogShader.setVec3("u_FogColor", env.VolumetricFog_Color);
    fogShader.setInt("u_StepCount", env.VolumetricFog_Steps);

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    Shader& blendShader = ShaderManager::Get("VolumetricFogBlend");
    blendShader.use();

    glBindTextureUnit(0, m_FogFramebuffer.GetColorAttachment(0));
    glBindTextureUnit(1, i_Depth);

    blendShader.setInt("u_FogTexture", 0);
    blendShader.setInt("gDepth", 1);
    blendShader.setVec2("u_TexelSize", glm::vec2(1.0f / static_cast<float>(ctx.viewportWidth), 1.0f / static_cast<float>(ctx.viewportHeight)));
    blendShader.setBool("u_ApplyBilateral", m_HalfResolution);

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glBlitFramebuffer(
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("VolumetricFogTexture", { RenderResourceType::Texture, m_FogFramebuffer.GetColorAttachment(0) });
}
