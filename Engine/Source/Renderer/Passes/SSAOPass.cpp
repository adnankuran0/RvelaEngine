#include "rvelapch.h"
#include "SSAOPass.h"
#include <random>

void SSAOPass::Init()
{
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    glGenTextures(1, &o_SsaoTexture);
    glBindTexture(GL_TEXTURE_2D, o_SsaoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_SsaoTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    GenerateSampleKernel();
    GenerateNoiseTexture();
}

SSAOPass::~SSAOPass()
{
    //TODO: Fill this function
}

void SSAOPass::Execute()
{
    Shader& ssaoShader = Renderer::GetSSAOShader();

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader.use();
    ssaoShader.setMat4("projection", ctx.camera->projection);
    glm::mat4 invProjection = glm::inverse(ctx.camera->projection);
    ssaoShader.setMat4("invProjection", invProjection); 
    ssaoShader.setInt("windowWidth", ctx.viewportWidth); 
    ssaoShader.setInt("windowHeight", ctx.viewportHeight);
    ssaoShader.setFloat("near", 0.1f);
    ssaoShader.setFloat("far", 100.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, i_Normal);
    ssaoShader.setInt("gNormal", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, i_Depth);
    ssaoShader.setInt("gDepth", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    ssaoShader.setInt("texNoise", 2);

    for (unsigned int i = 0; i < 64; ++i)
        ssaoShader.setVec3("samples[" + std::to_string(i) + "]", kernel[i]);
    Renderer::DrawFullScreenQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAOPass::GenerateSampleKernel()
{
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;

    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0f;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        kernel[i] = sample;
    }
}

void SSAOPass::GenerateNoiseTexture()
{
    std::uniform_real_distribution<float> randomFloats(-1.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> noise;

    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 n(randomFloats(generator), randomFloats(generator), 0.0f);
        noise.push_back(n);
    }

    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &noise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}