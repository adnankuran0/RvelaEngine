#include "rvelapch.h"
#include "ShadowPass.h"

GLuint ShadowPass::fbo = 0;
GLuint ShadowPass::depthMap = 0;
GLuint ShadowPass::pointFBO = 0;
GLuint ShadowPass::pointDepthMap = 0;
glm::mat4 ShadowPass::lightSpaceMatrix;
bool ShadowPass::isInitialized = false;

void ShadowPass::Execute()
{
    if (commands.empty() || !ctx.IsValid()) return;

    //DIRECTIONAL LIGHT SHADOW MAPPING
    if (ctx.directionalLight && ctx.directionalLight->castShadows)
    {
        Shader& shadowShader = Renderer::GetDirectionalShadowShader();
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

        if (ctx.directionalLight->reverseCullFace)
        {
            glCullFace(GL_BACK);
        }
        else
        {
            glCullFace(GL_FRONT);
        }
        
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

    //POINT LIGHT SHADOW MAPPING
    Shader& pointShadowShader = Renderer::GetPointShadowShader();
    pointShadowShader.use();
    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, pointDepthMap, 0);
    glClear(GL_DEPTH_BUFFER_BIT);   
    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);
    glDisable(GL_CULL_FACE);
    for (auto& light : ctx.pointLights)
    {
        if (!light.castShadows) continue;


        glm::vec3 lightPos = light.position;

        float near_plane = 0.1f;
        float far_plane = light.radius;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)POINT_SHADOW_WIDTH/ (float)POINT_SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        

        
        for (unsigned int i = 0; i < 6; ++i)
            pointShadowShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        pointShadowShader.setFloat("far_plane", far_plane);
        pointShadowShader.setVec3("lightPos", lightPos);
        pointShadowShader.setInt("shadowIndex", light.shadowIndex);
        for (auto& command : commands) {
            pointShadowShader.setMat4("model", command.transform.GetWorldMatrix());
            command.mesh.VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_CULL_FACE);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
    commands.clear();
}