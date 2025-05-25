#include "rvelapch.h"
#include "MeshPass.h"
#include "Core/Utils/TextureManager.h"
#include "Core/Utils/MaterialManager.h"
#include "../../RvelaLog.h"

void MeshPass::Execute() {
    if (commands.empty() || !ctx.IsValid()) return;

    Shader& shader = Renderer::GetDefaultShader();
    shader.use();

    shader.setBool("hasDirectionalLight", ctx.directionalLight.has_value());
    if (ctx.directionalLight) {
        shader.setVec3("directionalLight.direction", ctx.directionalLight->direction);
        shader.setVec3("directionalLight.color", ctx.directionalLight->color);
        shader.setFloat("directionalLight.intensity", ctx.directionalLight->intensity);
        shader.setBool("directionalLight.castShadows", ctx.directionalLight->castShadows);
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        shader.setInt("shadowMap", 6);
        glActiveTexture(GL_TEXTURE0 + 6);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        GLenum err = glGetError();
        //if (err != GL_NO_ERROR) LOG_ERROR << "OpenGL Error after directional light setup: " << err;
    }

    // Point light ayarları
    
    const int MAX_POINT_LIGHTS = 20;
    int pointLightCount = std::min(static_cast<int>(ctx.pointLights.size()), MAX_POINT_LIGHTS);
   
    for (int i = 0; i < pointLightCount; ++i) {
        std::string baseName = "pointLights[" + std::to_string(i) + "]";
        shader.setVec3(baseName + ".position", ctx.pointLights[i].position);
        shader.setVec3(baseName + ".color", ctx.pointLights[i].color);
        shader.setFloat(baseName + ".intensity", ctx.pointLights[i].intensity);
        shader.setFloat(baseName + ".radius", ctx.pointLights[i].radius);
        shader.setBool(baseName + ".castShadows", ctx.pointLights[i].castShadows);
    }

    shader.setInt("pointLightCount", pointLightCount);

    shader.setVec3("camPos", ctx.camera->Position);

    for (auto& command : commands) {
        auto& material = command.material.material;
        if (!material) continue;

        shader.setVec3("UVScale", material->UVScale);
        shader.setVec3("UVOffset", material->UVOffset);
        shader.setVec3("albedoColor", material->albedoColor);
        shader.setFloat("metallicValue", material->metallic);
        shader.setFloat("roughnessValue", material->roughness);
        shader.setFloat("aoValue", material->ao);

        struct MapInfo {
            Path path;
            std::string uniformName;
            int slot;
            std::string useUniform;
        };

        std::vector<MapInfo> maps = {
            { material->albedoMapPath,    "albedoMap",    0, "useAlbedoMap" },
            { material->normalMapPath,    "normalMap",    1, "useNormalMap" },
            { material->metallicMapPath,  "metallicMap",  2, "useMetallicMap" },
            { material->roughnessMapPath, "roughnessMap", 3, "useRoughnessMap" },
            { material->aoMapPath,        "aoMap",        4, "useAOMap" },
            { material->heightMapPath,    "heightMap",    5, "useHeightMap" },
        };

        for (const auto& map : maps) {
            bool hasMap = map.path.IsValid();
            shader.setBool(map.useUniform, hasMap);
            if (hasMap) {
                auto tex = TextureManager::LoadOrGetTexture(map.path);
                if (tex) {
                    shader.setInt(map.uniformName, map.slot);
                    tex->Bind(map.slot);
                }
            }
        }

        shader.setMat4("model", command.transform.GetWorldMatrix());
        shader.setMat4("view", ctx.camera->GetViewMatrix());
        shader.setMat4("projection", ctx.camera->projection);

        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOG_ERROR << "OpenGL Error: " << err;
        }
    }

    commands.clear();
}
