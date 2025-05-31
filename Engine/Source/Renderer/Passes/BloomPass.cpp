#include "rvelapch.h"
#include "BloomPass.h"
#include <algorithm>



void BloomPass::Init()
{
    downsampleFBOs.resize(mipLevels);
    downsampleTextures.resize(mipLevels);
    upsampleFBOs.resize(mipLevels);
    upsampleTextures.resize(mipLevels);

    for (int i = 0; i < mipLevels; ++i)
    {
        int w = std::max((unsigned int)1, ctx.viewportWidth >> i);
        int h = std::max((unsigned int)1, ctx.viewportHeight >> i);

        GLuint texDown, fboDown, texUp, fboUp;

        glGenTextures(1, &texDown);
        glBindTexture(GL_TEXTURE_2D, texDown);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &fboDown);
        glBindFramebuffer(GL_FRAMEBUFFER, fboDown);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texDown, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, fboDown);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Downsample framebuffer incomplete at mip level " << i << std::endl;
        }

        glGenTextures(1, &texUp);
        glBindTexture(GL_TEXTURE_2D, texUp);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &fboUp);
        glBindFramebuffer(GL_FRAMEBUFFER, fboUp);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texUp, 0);

        downsampleTextures[i] = texDown;
        downsampleFBOs[i] = fboDown;
        upsampleTextures[i] = texUp;
        upsampleFBOs[i] = fboUp;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

BloomPass::~BloomPass()
{
    //TODO: Fill this function
}

void BloomPass::Execute()
{
    glDisable(GL_DEPTH_TEST);

    Shader& downsampleShader = Renderer::GetDownsampleShader();
    Shader& upsampleShader = Renderer::GetUpsampleShader();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, i_BrightTexture);

    for (int i = 0; i < mipLevels; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, downsampleFBOs[i]);
        int w = std::max(1, (int)ctx.viewportWidth >> i);
        int h = std::max(1, (int)ctx.viewportHeight >> i);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        downsampleShader.use();
        downsampleShader.setInt("u_Texture", 0);
        downsampleShader.setVec2("u_TexelSize", glm::vec2(1.0f / w, 1.0f / h));

        if (i == 0)
            glBindTexture(GL_TEXTURE_2D, i_BrightTexture);
        else
            glBindTexture(GL_TEXTURE_2D, downsampleTextures[i - 1]);

        Renderer::DrawFullScreenQuad();
    }

    for (int i = mipLevels - 2; i >= 0; --i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, upsampleFBOs[i]);
        int w = std::max(1, (int)ctx.viewportWidth >> i);
        int h = std::max(1, (int)ctx.viewportHeight >> i);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        upsampleShader.use();
        upsampleShader.setInt("u_UpsampleTex", 0);
        upsampleShader.setInt("u_BaseTex", 1);
        upsampleShader.setVec2("u_TexelSize", glm::vec2(1.0f / w, 1.0f / h));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, upsampleTextures[i + 1]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, downsampleTextures[i]);

        Renderer::DrawFullScreenQuad();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    o_BlurredTexture = upsampleTextures[0];

    glEnable(GL_DEPTH_TEST);
}