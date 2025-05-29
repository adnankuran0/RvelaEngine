#include "rvelapch.h"
#include "BloomPass.h"
#include <algorithm>

bool BloomPass::isInitialized = false;
GLuint BloomPass::quadVAO = 0;
GLuint BloomPass::quadVBO = 0;
std::vector<GLuint> BloomPass::downsampleFBOs;
std::vector<GLuint> BloomPass::downsampleTextures;
std::vector<GLuint> BloomPass::upsampleFBOs;
std::vector<GLuint> BloomPass::upsampleTextures;
GLuint BloomPass::blurredTexture = 0;

void BloomPass::Execute()
{
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadVAO);

    Shader& downsampleShader = Renderer::GetDownsampleShader();
    Shader& upsampleShader = Renderer::GetUpsampleShader();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, brightTexture);

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
            glBindTexture(GL_TEXTURE_2D, brightTexture);
        else
            glBindTexture(GL_TEXTURE_2D, downsampleTextures[i - 1]);

        glDrawArrays(GL_TRIANGLES, 0, 6);
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

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    blurredTexture = upsampleTextures[0];

    glEnable(GL_DEPTH_TEST);
}