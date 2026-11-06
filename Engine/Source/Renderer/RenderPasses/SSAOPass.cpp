#include "rvelapch.h"
#include "SSAOPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

constexpr int NOISE_SIZE = 16;

void SSAOPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    m_HalfResolution = ctx.environment ? ctx.environment->SSAO_HalfResolution : true;
    uint32_t targetWidth = m_HalfResolution ? std::max(1u, ctx.viewportWidth / 2) : std::max(1u, ctx.viewportWidth);
    uint32_t targetHeight = m_HalfResolution ? std::max(1u, ctx.viewportHeight / 2) : std::max(1u, ctx.viewportHeight);

    FramebufferDesc desc;
    desc.width = targetWidth;
    desc.height = targetHeight;
    desc.colorAttachments = {
        { FramebufferTextureFormat::R16F, FramebufferFilterMode::Linear }
    };
    desc.hasDepth = false;

    m_Framebuffer = Framebuffer(desc);

    GenerateNoiseTexture();

    frame.registry.Register("SSAOTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

void SSAOPass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    uint32_t targetWidth = m_HalfResolution ? std::max(1u, width / 2) : std::max(1u, width);
    uint32_t targetHeight = m_HalfResolution ? std::max(1u, height / 2) : std::max(1u, height);

    if (m_Framebuffer.GetWidth() == targetWidth && m_Framebuffer.GetHeight() == targetHeight)
        return;

    m_Framebuffer.Resize(targetWidth, targetHeight);
    frame.registry.Register("SSAOTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

SSAOPass::~SSAOPass()
{
    glDeleteTextures(1, &noiseTexture);
}

void SSAOPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.environment->SSAO) return;

    bool halfRes = ctx.environment->SSAO_HalfResolution;
    if (halfRes != m_HalfResolution)
    {
        m_HalfResolution = halfRes;
    }
    uint32_t targetWidth = m_HalfResolution ? std::max(1u, ctx.viewportWidth / 2) : std::max(1u, ctx.viewportWidth);
    uint32_t targetHeight = m_HalfResolution ? std::max(1u, ctx.viewportHeight / 2) : std::max(1u, ctx.viewportHeight);

    if (m_Framebuffer.GetWidth() != targetWidth || m_Framebuffer.GetHeight() != targetHeight)
    {
        m_Framebuffer.Resize(targetWidth, targetHeight);
        frame.registry.Register("SSAOTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
    }

    auto i_Normal = frame.registry.Get("NormalTexture")->id;
    auto i_Depth = frame.registry.Get("DepthTexture")->id;
    auto& env = *ctx.environment;

    Shader& ssaoShader = ShaderManager::Get("SSAO");

    m_Framebuffer.BindViewport();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader.use();
    ssaoShader.setFloat("radius", env.SSAO_Radius);
    ssaoShader.setFloat("bias", env.SSAO_Bias);
    ssaoShader.setFloat("intensity", env.SSAO_Intensity);

    glBindTextureUnit(0, i_Normal);
    glBindTextureUnit(1, i_Depth);
    glBindTextureUnit(2, noiseTexture);

    ssaoShader.setInt("gNormal", 0);
    ssaoShader.setInt("gDepth", 1);
    ssaoShader.setInt("texNoise", 2);

    Renderer::DrawFullScreenQuad();

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(2, 0);

    Framebuffer::BindDefault();
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}

void SSAOPass::GenerateNoiseTexture()
{
    std::uniform_real_distribution<float> randomFloats(-1.0f, 1.0f);
    std::default_random_engine generator;

    std::array<glm::vec3, NOISE_SIZE> noise;
    for (auto& n : noise) {
        n = glm::vec3(randomFloats(generator), randomFloats(generator), 0.0f);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &noiseTexture);
    glTextureStorage2D(noiseTexture, 1, GL_RGB16F, 4, 4);
    glTextureSubImage2D(noiseTexture, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, noise.data());

    glTextureParameteri(noiseTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(noiseTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(noiseTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(noiseTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);
}
