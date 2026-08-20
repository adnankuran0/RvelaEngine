#include "rvelapch.h"
#include "SSAOPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderManager.h"

using namespace rv;

constexpr int KERNEL_SIZE = 32;
constexpr int NOISE_SIZE = 16;

void SSAOPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = ctx.viewportWidth / 2;
    desc.height = ctx.viewportHeight / 2;
    desc.colorAttachments = {
        { FramebufferTextureFormat::R16F, FramebufferFilterMode::Nearest }
    };
    desc.hasDepth = false;

    m_Framebuffer = Framebuffer(desc);

    GenerateSampleKernel();
    GenerateNoiseTexture();

    frame.registry.Register("SSAOTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

SSAOPass::~SSAOPass()
{
    glDeleteTextures(1, &noiseTexture);
}

void SSAOPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (!ctx.environment->SSAO) return;

    auto i_Normal = frame.registry.Get("NormalTexture")->id;
    auto i_Depth = frame.registry.Get("DepthTexture")->id;
    auto& env = *ctx.environment;

    Shader& ssaoShader = ShaderManager::Get("SSAO");

    m_Framebuffer.BindViewport();
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

    for (unsigned int i = 0; i < KERNEL_SIZE; ++i)
        ssaoShader.setVec3("samples[" + std::to_string(i) + "]", kernel.at(i));

    Renderer::DrawFullScreenQuad();
    Framebuffer::BindDefault();
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}

void SSAOPass::GenerateSampleKernel()
{
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    kernel.reserve(KERNEL_SIZE);

    for (unsigned int i = 0; i < KERNEL_SIZE; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        float scale = static_cast<float>(i) / KERNEL_SIZE;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;

        kernel.push_back(sample);
    }
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