#include "rvelapch.h"
#include "MotionBlurPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/ShaderManager.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Renderer/Camera.h"
#include "Core/Time.h"

using namespace rv;

void MotionBlurPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = ctx.viewportWidth;
    desc.height = ctx.viewportHeight;
    desc.hasDepth = false;
    desc.colorAttachments = {
        { FramebufferTextureFormat::RGBA16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
    };

    m_Framebuffer = Framebuffer(desc);
    frame.registry.Register("MotionBlurTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

void MotionBlurPass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    if (m_Framebuffer.GetWidth() == width && m_Framebuffer.GetHeight() == height)
        return;

    m_Framebuffer.Resize(width, height);
    frame.registry.Register("MotionBlurTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

void MotionBlurPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.camera)
        return;

    glm::mat4 currentView = ctx.camera->GetViewMatrix();
    glm::mat4 currentProj = ctx.camera->GetProjectionMatrix();
    glm::mat4 currentViewProj = currentProj * currentView;

    if (!ctx.camera->prevViewProjValid)
    {
        ctx.camera->prevViewProj = currentViewProj;
        ctx.camera->prevViewProjValid = true;
    }

    auto* screenRes = frame.registry.Get("ScreenTexture");
    if (!screenRes || screenRes->id == 0)
        return;

    GLuint i_ScreenTexture = screenRes->id;

    auto* depthRes = frame.registry.Get("DepthTexture");
    if (!depthRes || depthRes->id == 0)
        return;

    GLuint i_DepthTexture = depthRes->id;

    auto& env = *ctx.environment;

    float dt = Time::GetDeltaTime();
    if (dt > 0.1f)
    {
        ctx.camera->prevViewProj = currentViewProj;
    }

    if (!env.MotionBlur || env.MotionBlur_Intensity <= 0.0f)
    {
        frame.registry.Register("MotionBlurTexture", { RenderResourceType::Texture, i_ScreenTexture });
        ctx.camera->prevViewProj = currentViewProj;
        return;
    }

    if (m_Framebuffer.GetWidth() != ctx.viewportWidth || m_Framebuffer.GetHeight() != ctx.viewportHeight)
    {
        OnResize(ctx.viewportWidth, ctx.viewportHeight, frame);
    }

    frame.registry.Register("MotionBlurTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });

    glDisable(GL_DEPTH_TEST);
    m_Framebuffer.BindViewport();
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& mbShader = ShaderManager::Get("MotionBlur");
    mbShader.use();

    glm::mat4 invViewProj = glm::inverse(currentViewProj);

    mbShader.setMat4("u_InvViewProj", invViewProj);
    mbShader.setMat4("u_PrevViewProj", ctx.camera->prevViewProj);
    mbShader.setFloat("u_Intensity", env.MotionBlur_Intensity);
    mbShader.setInt("u_Samples", env.MotionBlur_Samples);
    mbShader.setFloat("u_MaxVelocity", env.MotionBlur_MaxVelocity);

    mbShader.setInt("u_ScreenTexture", 0);
    mbShader.setInt("u_DepthTexture", 1);

    glBindTextureUnit(0, i_ScreenTexture);
    glBindTextureUnit(1, i_DepthTexture);

    Renderer::DrawFullScreenQuad();

    glEnable(GL_DEPTH_TEST);
    Framebuffer::BindDefault();

    ctx.camera->prevViewProj = currentViewProj;
}
