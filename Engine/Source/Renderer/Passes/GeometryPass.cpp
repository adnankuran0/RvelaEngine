#include "rvelapch.h"
#include "GeometryPass.h"

GLuint GeometryPass::gBuffer = 0;
GLuint GeometryPass::gNormal = 0;
GLuint GeometryPass::gDepth = 0;
GLuint GeometryPass::gMetallic = 0;
GLuint GeometryPass::gRoughness = 0;
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
        auto& material = command.material.material;

        geometryShader.setMat4("model", command.transform.GetWorldMatrix());
        auto& roughnessMapPath = material->roughnessMapPath;
        auto& metallicMapPath = material->metallicMapPath;

        if (roughnessMapPath.IsValid())
        {
            auto roughnessTexture = TextureManager::LoadOrGetTexture(roughnessMapPath);
            geometryShader.setBool("useRoughnessMap", true);
            geometryShader.setInt("roughnessMap", 0);
            roughnessTexture->Bind(0);
        }
        else
        {
            geometryShader.setBool("useRoughnessMap", false);
            geometryShader.setFloat("roughness", material->roughness);
        }

        if (metallicMapPath.IsValid())
        {
            auto metallicTexture = TextureManager::LoadOrGetTexture(metallicMapPath);
            geometryShader.setBool("useMetallicMap", true);
            geometryShader.setInt("metallicMap", 1);
            metallicTexture->Bind(1);
        }
        else
        {
            geometryShader.setBool("useMetallicMap", false);
            geometryShader.setFloat("metallic", material->metallic);
        }


        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    commands.clear();
}