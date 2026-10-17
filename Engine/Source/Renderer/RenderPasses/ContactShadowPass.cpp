#include "rvelapch.h"
#include "ContactShadowPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

void ContactShadowPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    uint32_t w = std::max(1u, ctx.viewportWidth);
    uint32_t h = std::max(1u, ctx.viewportHeight);

    FramebufferDesc desc;
    desc.width = w;
    desc.height = h;
    desc.colorAttachments = {
        { FramebufferTextureFormat::R16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
    };
    desc.hasDepth = false;

    m_RawFramebuffer = Framebuffer(desc);
    m_BlurFramebuffer = Framebuffer(desc);

    frame.registry.Register("ContactShadowTexture", { RenderResourceType::Texture, m_BlurFramebuffer.GetColorAttachment(0) });
    frame.registry.Register("RawContactShadowTexture", { RenderResourceType::Texture, m_RawFramebuffer.GetColorAttachment(0) });
}

void ContactShadowPass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    uint32_t w = std::max(1u, width);
    uint32_t h = std::max(1u, height);

    if (m_BlurFramebuffer.GetWidth() == w && m_BlurFramebuffer.GetHeight() == h)
        return;

    m_RawFramebuffer.Resize(w, h);
    m_BlurFramebuffer.Resize(w, h);

    frame.registry.Register("ContactShadowTexture", { RenderResourceType::Texture, m_BlurFramebuffer.GetColorAttachment(0) });
    frame.registry.Register("RawContactShadowTexture", { RenderResourceType::Texture, m_RawFramebuffer.GetColorAttachment(0) });
}

void ContactShadowPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (m_BlurFramebuffer.GetWidth() != ctx.viewportWidth || m_BlurFramebuffer.GetHeight() != ctx.viewportHeight)
    {
        OnResize(ctx.viewportWidth, ctx.viewportHeight, frame);
    }

    auto& env = *ctx.environment;
    bool enabled = env.ContactShadows && ctx.directionalLight.has_value() && ctx.directionalLight->castShadows && ctx.directionalLight->contactShadows;

    if (!enabled)
    {
        m_BlurFramebuffer.BindViewport();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        Framebuffer::BindDefault();
        glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
        return;
    }

    auto* depthRes = frame.registry.Get("DepthTexture");
    auto* normalRes = frame.registry.Get("NormalTexture");
    if (!depthRes || !normalRes || depthRes->id == 0 || normalRes->id == 0)
        return;

    const auto& dirLight = ctx.directionalLight.value();
    glm::vec3 lightDirWorld = glm::normalize(-dirLight.direction);
    glm::mat4 viewMatrix = ctx.camera ? ctx.camera->GetViewMatrix() : glm::mat4(1.0f);
    glm::vec3 lightDirView = glm::normalize(glm::mat3(viewMatrix) * lightDirWorld);

    m_RawFramebuffer.BindViewport();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& contactShader = ShaderManager::Get("ContactShadow");
    contactShader.use();

    glBindTextureUnit(0, depthRes->id);
    glBindTextureUnit(1, normalRes->id);
    contactShader.setInt("gDepth", 0);
    contactShader.setInt("gNormal", 1);

    contactShader.setVec3("u_LightDir", lightDirView);
    contactShader.setFloat("u_Distance", env.ContactShadows_Distance);
    contactShader.setFloat("u_Thickness", env.ContactShadows_Thickness);
    contactShader.setFloat("u_Bias", env.ContactShadows_Bias);
    contactShader.setFloat("u_Intensity", env.ContactShadows_Intensity);
    contactShader.setInt("u_Steps", env.ContactShadows_Steps);

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);

    m_BlurFramebuffer.BindViewport();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& blurShader = ShaderManager::Get("ContactShadowBlur");
    blurShader.use();

    glBindTextureUnit(0, m_RawFramebuffer.GetColorAttachment(0));
    glBindTextureUnit(1, depthRes->id);
    glBindTextureUnit(2, normalRes->id);
    blurShader.setInt("u_ContactShadowTexture", 0);
    blurShader.setInt("gDepth", 1);
    blurShader.setInt("gNormal", 2);
    blurShader.setVec2("u_TexelSize", glm::vec2(1.0f / static_cast<float>(ctx.viewportWidth), 1.0f / static_cast<float>(ctx.viewportHeight)));

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);

    Framebuffer::BindDefault();
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}
