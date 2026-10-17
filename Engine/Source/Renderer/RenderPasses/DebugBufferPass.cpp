#include "rvelapch.h"
#include "DebugBufferPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/Renderer.h"
#include "Renderer/Camera.h"

using namespace rv;

DebugBufferPass::DebugBufferPass(RenderPipeline* pipeline)
    : m_Pipeline(pipeline)
{
}

DebugBufferPass::~DebugBufferPass()
{
    DestroyDummyTextures();
}

void DebugBufferPass::CreateDummyTextures()
{
    if (m_DummyTex2D == 0)
    {
        glGenTextures(1, &m_DummyTex2D);
        glBindTexture(GL_TEXTURE_2D, m_DummyTex2D);
        float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_FLOAT, black);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    if (m_DummyTexArray == 0)
    {
        glGenTextures(1, &m_DummyTexArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_DummyTexArray);
        float depth = 1.0f;
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, 1, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    if (m_DummyTexUInt == 0)
    {
        glGenTextures(1, &m_DummyTexUInt);
        glBindTexture(GL_TEXTURE_2D, m_DummyTexUInt);
        uint32_t zero = 0;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, 1, 1, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    if (m_DummyTexCubeArray == 0)
    {
        glGenTextures(1, &m_DummyTexCubeArray);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_DummyTexCubeArray);
        glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_DEPTH_COMPONENT32F, 1, 1, 6, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void DebugBufferPass::DestroyDummyTextures()
{
    if (m_DummyTex2D != 0)
    {
        glDeleteTextures(1, &m_DummyTex2D);
        m_DummyTex2D = 0;
    }
    if (m_DummyTexArray != 0)
    {
        glDeleteTextures(1, &m_DummyTexArray);
        m_DummyTexArray = 0;
    }
    if (m_DummyTexUInt != 0)
    {
        glDeleteTextures(1, &m_DummyTexUInt);
        m_DummyTexUInt = 0;
    }
    if (m_DummyTexCubeArray != 0)
    {
        glDeleteTextures(1, &m_DummyTexCubeArray);
        m_DummyTexCubeArray = 0;
    }
}

void DebugBufferPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    CreateDummyTextures();

    FramebufferDesc desc;
    desc.width = ctx.viewportWidth > 0 ? ctx.viewportWidth : 1920;
    desc.height = ctx.viewportHeight > 0 ? ctx.viewportHeight : 1080;
    desc.hasDepth = false;
    desc.colorAttachments = {
        { FramebufferTextureFormat::RGBA16F, FramebufferFilterMode::Linear, FramebufferWrapMode::ClampToEdge }
    };

    m_Framebuffer = Framebuffer(desc);
    frame.registry.Register("DebugBufferTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

void DebugBufferPass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    if (width == 0 || height == 0) return;
    if (m_Framebuffer.GetWidth() == width && m_Framebuffer.GetHeight() == height) return;

    m_Framebuffer.Resize(width, height);
    frame.registry.Register("DebugBufferTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

void DebugBufferPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!m_Pipeline) return;

    DebugBufferMode mode = m_Pipeline->GetDebugBufferMode();
    if (mode == DebugBufferMode::None)
        return;

    if (m_Framebuffer.GetWidth() != ctx.viewportWidth || m_Framebuffer.GetHeight() != ctx.viewportHeight)
    {
        OnResize(ctx.viewportWidth, ctx.viewportHeight, frame);
    }

    auto& shader = ShaderManager::Get("DebugBuffer");
    shader.use();

    shader.setInt("u_Mode", static_cast<int>(mode));
    shader.setInt("u_Layer", m_Pipeline->GetDebugBufferLayer());
    shader.setInt("u_Face", m_Pipeline->GetDebugBufferFace());
    shader.setFloat("u_Near", ctx.camera ? ctx.camera->NearClip : 0.1f);
    shader.setFloat("u_Far", ctx.camera ? ctx.camera->FarClip : 1000.0f);

    shader.setInt("u_Texture2D", 0);
    shader.setInt("u_TextureArray", 1);
    shader.setInt("u_TextureUInt", 2);
    shader.setInt("u_TextureCubeArray", 3);

    glBindTextureUnit(0, m_DummyTex2D);
    glBindTextureUnit(1, m_DummyTexArray);
    glBindTextureUnit(2, m_DummyTexUInt);
    glBindTextureUnit(3, m_DummyTexCubeArray);

    GLuint tex2D = 0;
    GLuint texArray = 0;
    GLuint texUInt = 0;
    GLuint texCubeArray = 0;

    switch (mode)
    {
    case DebugBufferMode::Normal:
        if (auto* r = frame.registry.Get("NormalTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::Depth:
        if (auto* r = frame.registry.Get("DepthTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::Roughness:
        if (auto* r = frame.registry.Get("RoughnessTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::Metallic:
        if (auto* r = frame.registry.Get("MetallicTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::SSAO:
        if (auto* r = frame.registry.Get("SSAOTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::SSGI:
        if (auto* r = frame.registry.Get("SSGITexture")) tex2D = r->id;
        break;
    case DebugBufferMode::SSR:
        if (auto* r = frame.registry.Get("SSRTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::DirectionalShadow:
        if (auto* r = frame.registry.Get("DirectionalShadowMap")) texArray = r->id;
        break;
    case DebugBufferMode::SpotShadow:
        if (auto* r = frame.registry.Get("SpotShadowMap")) texArray = r->id;
        break;
    case DebugBufferMode::Bloom:
        if (auto* r = frame.registry.Get("BloomTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::BrightPass:
        if (auto* r = frame.registry.Get("BrightTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::EntityID:
        if (auto* r = frame.registry.Get("EntityTexture")) texUInt = r->id;
        break;
    case DebugBufferMode::ScreenColor:
        if (auto* r = frame.registry.Get("ScreenTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::ContactShadows:
        if (auto* r = frame.registry.Get("ContactShadowTexture")) tex2D = r->id;
        break;
    case DebugBufferMode::PointShadow:
        if (auto* r = frame.registry.Get("PointShadowMap")) texCubeArray = r->id;
        break;
    case DebugBufferMode::MotionBlur:
        if (auto* r = frame.registry.Get("MotionBlurTexture")) tex2D = r->id;
        break;
    default:
        break;
    }

    if (tex2D != 0)
        glBindTextureUnit(0, tex2D);
    if (texArray != 0)
        glBindTextureUnit(1, texArray);
    if (texUInt != 0)
        glBindTextureUnit(2, texUInt);
    if (texCubeArray != 0)
        glBindTextureUnit(3, texCubeArray);

    m_Framebuffer.BindViewport();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);
    glBindTextureUnit(3, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("DebugBufferTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}
