#include "rvelapch.h"
#include "ShadowPass.h"

GLuint ShadowPass::fbo = 0;
GLuint ShadowPass::depthMap = 0;
glm::mat4 ShadowPass::lightSpaceMatrix;
bool ShadowPass::isInitialized = false;

void ShadowPass::Execute()
{
    if (commands.empty() || !ctx.IsValid()) return;

    if (ctx.directionalLight && ctx.directionalLight->castShadows)
    {
        Shader& shadowShader = Renderer::GetShadowShader();
        shadowShader.use();

        glm::vec3 lightDir = ctx.directionalLight->direction;
        glm::vec3 sceneCenter = ctx.camera->Position;
        glm::mat4 lightView = glm::lookAt(sceneCenter - lightDir * 50.0f, sceneCenter, glm::vec3(0, 1, 0));
        glm::mat4 lightProjection = glm::ortho(-35.0f, 35.0f, -35.0f, 35.0f, 0.01f, 120.0f);
        lightSpaceMatrix = lightProjection * lightView;

        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 2.0f);
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);

        for (auto& command : commands) {
            shadowShader.setMat4("model", command.transform.GetWorldMatrix());
            command.mesh.VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glCullFace(GL_BACK);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Shader& pointShadowShader = Renderer::GetPointShadowShader();
    pointShadowShader.use();

    glDisable(GL_CULL_FACE);
    for (auto& light : ctx.pointLights)
    {
        if (!light.castShadows) continue;
        glm::vec3 lightPos = light.position;

        float near_plane = 1.0f;
        float far_plane = light.radius;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)light.shadowWidth / (float)light.shadowHeight, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        glViewport(0, 0, light.shadowWidth, light.shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, light.depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        pointShadowShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            pointShadowShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        pointShadowShader.setFloat("far_plane", far_plane);
        pointShadowShader.setVec3("lightPos", lightPos);
        for (auto& command : commands) {
            pointShadowShader.setMat4("model", command.transform.GetWorldMatrix());
            command.mesh.VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glEnable(GL_CULL_FACE);

    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
    commands.clear();
}