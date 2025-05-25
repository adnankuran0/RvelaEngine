#include "rvelapch.h"
#include "ShadowPass.h"

GLuint ShadowPass::fbo = 0;
GLuint ShadowPass::depthMap = 0;
glm::mat4 ShadowPass::lightSpaceMatrix;
bool ShadowPass::isInitialized = false;

void ShadowPass::Execute()
{
    if (commands.empty() || !ctx.IsValid() || !ctx.directionalLight.has_value()) return;

    Shader& shadowShader = Renderer::GetShadowShader();
    shadowShader.use();

    if (ctx.directionalLight->castShadows)
    {
        glm::vec3 lightDir = ctx.directionalLight->direction;
        glm::vec3 sceneCenter = ctx.camera->Position;
        glm::mat4 lightView = glm::lookAt(sceneCenter - lightDir * 50.0f, sceneCenter, glm::vec3(0, 1, 0));

        float orthoSize = 35.0f;
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 120.0f);
        lightSpaceMatrix = lightProjection * lightView;
        shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Enable and adjust polygon offset
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.5f, 1.0f); // Reduced values

        // Enable front face culling
        glEnable(GL_CULL_FACE);

        for (auto& command : commands) {
            shadowShader.setMat4("model", command.transform.GetWorldMatrix());
            command.mesh.VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        // Reset to default culling and disable polygon offset
        glCullFace(GL_BACK);
        glDisable(GL_POLYGON_OFFSET_FILL);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
    }



    commands.clear();
}