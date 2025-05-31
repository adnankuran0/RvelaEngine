#include "rvelapch.h"
#include "SSRPass.h"

void SSRPass::Init()
{
    glGenFramebuffers(1, &ssrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);

    glGenTextures(1, &o_SsrTexture);
    glBindTexture(GL_TEXTURE_2D, o_SsrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_SsrTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSR framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

SSRPass::~SSRPass()
{
    //TODO: Fill this function
}

void SSRPass::Execute()
{
    Shader& ssrShader = Renderer::GetSSRShader();
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, ssrFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    ssrShader.use();

    glm::mat4 viewMatrix = ctx.camera->GetViewMatrix();
    glm::mat4 projectionMatrix = ctx.camera->projection;

    ssrShader.setMat4("uViewMatrix", viewMatrix);
    ssrShader.setMat4("uProjectionMatrix", projectionMatrix);
    ssrShader.setMat4("uInverseProjectionMatrix", glm::inverse(projectionMatrix));
    ssrShader.setMat4("uInverseViewMatrix", glm::inverse(viewMatrix));

    ssrShader.setFloat("near", 0.1f);
    ssrShader.setFloat("far", 100.0f);
    ssrShader.setFloat("uTime", (float)Time::GetCurrentTime());
    ssrShader.setVec3("uCameraPos", ctx.camera->Position);
    ssrShader.setFloat("uMaxRayDistance", 100.0f);


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, i_Depth);
    ssrShader.setInt("uDepthTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, i_Normal);
    ssrShader.setInt("uNormalTexture", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, i_Roughness);
    ssrShader.setInt("uRoughnessTexture", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, i_Metallic);
    ssrShader.setInt("uMetallicTexture", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, i_Screen);
    ssrShader.setInt("uScreenTexture", 4);

    Renderer::DrawFullScreenQuad();

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
