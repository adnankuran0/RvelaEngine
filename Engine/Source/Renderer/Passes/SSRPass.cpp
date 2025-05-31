#include "rvelapch.h"
#include "SSRPass.h"

bool SSRPass::isInitialized = false;
GLuint SSRPass::ssrFBO = 0;
GLuint SSRPass::ssrTexture = 0;

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
    glBindTexture(GL_TEXTURE_2D, gDepth);
    ssrShader.setInt("uDepthTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    ssrShader.setInt("uNormalTexture", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gRoughness);
    ssrShader.setInt("uRoughnessTexture", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gMetallic);
    ssrShader.setInt("uMetallicTexture", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gScreen);
    ssrShader.setInt("uScreenTexture", 4);

    Renderer::DrawFullScreenQuad();

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}
