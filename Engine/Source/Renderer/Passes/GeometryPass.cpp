#include "rvelapch.h"
#include "GeometryPass.h"

GLuint GeometryPass::gBuffer = 0;
GLuint GeometryPass::gNormal = 0;
GLuint GeometryPass::gDepth = 0;
bool GeometryPass::isInitialized = false;

void GeometryPass::Execute()
{
    Shader& geometryShader = Renderer::GetGeometryShader();

    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    geometryShader.use();

    geometryShader.setMat4("view", ctx.camera->GetViewMatrix());
    geometryShader.setMat4("projection", ctx.camera->projection);

    for (auto& command : commands) {
        geometryShader.setMat4("model", command.transform.GetWorldMatrix());
        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    commands.clear();
}