#include "rvelapch.h"
#include "SSAOPass.h"
#include <random>
#include <array>

// Pre-declare constants
namespace {
    constexpr int KERNEL_SIZE = 64;
    constexpr int NOISE_SIZE = 16;
    constexpr GLenum SSAO_TEXTURE_FORMAT = GL_R16F; // More efficient than GL_RED/GL_FLOAT
    constexpr float NEAR_PLANE = 0.1f;
    constexpr float FAR_PLANE = 100.0f;
}

void SSAOPass::Init()
{
    // Framebuffer setup
    glCreateFramebuffers(1, &ssaoFBO);

    // Texture setup using modern DSA (Direct State Access) functions where available
    glCreateTextures(GL_TEXTURE_2D, 1, &o_SsaoTexture);
    glTextureStorage2D(o_SsaoTexture, 1, SSAO_TEXTURE_FORMAT, ctx.viewportWidth, ctx.viewportHeight);
    glTextureParameteri(o_SsaoTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(o_SsaoTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Framebuffer attachment
    glNamedFramebufferTexture(ssaoFBO, GL_COLOR_ATTACHMENT0, o_SsaoTexture, 0);

    if (glCheckNamedFramebufferStatus(ssaoFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "SSAO framebuffer not complete!" << std::endl;

    GenerateSampleKernel();
    GenerateNoiseTexture();
}

SSAOPass::~SSAOPass()
{
    glDeleteTextures(1, &o_SsaoTexture);
    glDeleteTextures(1, &noiseTexture);
    glDeleteFramebuffers(1, &ssaoFBO);
}

void SSAOPass::Execute()
{
    Shader& ssaoShader = Renderer::GetSSAOShader();

    // Pre-calculate matrices
    const glm::mat4& projection = ctx.camera->projection;
    const glm::mat4 invProjection = glm::inverse(projection);

    // Set up framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    // Shader setup
    ssaoShader.use();
    ssaoShader.setMat4("projection", projection);
    ssaoShader.setMat4("invProjection", invProjection);
    ssaoShader.setVec2("windowSize", glm::vec2(ctx.viewportWidth, ctx.viewportHeight));
    ssaoShader.setFloat("near", NEAR_PLANE);
    ssaoShader.setFloat("far", FAR_PLANE);

    // Texture bindings
    glBindTextureUnit(0, i_Normal);
    glBindTextureUnit(1, i_Depth);
    glBindTextureUnit(2, noiseTexture);

    ssaoShader.setInt("gNormal", 0);
    ssaoShader.setInt("gDepth", 1);
    ssaoShader.setInt("texNoise", 2);

    for (unsigned int i = 0; i < 64; ++i)
        ssaoShader.setVec3("samples[" + std::to_string(i) + "]", kernel->at(i));

    Renderer::DrawFullScreenQuad();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAOPass::GenerateSampleKernel()
{
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    kernel->reserve(KERNEL_SIZE); 

    for (unsigned int i = 0; i < KERNEL_SIZE; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // Improved distribution
        float scale = static_cast<float>(i) / KERNEL_SIZE;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;

        kernel->push_back(sample);
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