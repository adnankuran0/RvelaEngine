#include "rvelapch.h"
#include "MeshPass.h"
#include "Core/Utils/TextureManager.h"
#include "Core/Utils/MaterialManager.h"
#include "../RvelaLog.h"

void MeshPass::Execute() {
    if (!camera) return;

    Shader& shader = Renderer::GetDefaultShader();
    shader.use();

    shader.setBool("hasDirectionalLight", directionalLight != nullptr);
    if (directionalLight) {
        shader.setVec3("directionalLight.direction", directionalLight->direction);
        shader.setVec3("directionalLight.color", directionalLight->color);
        shader.setFloat("directionalLight.intensity", directionalLight->intensity);
        shader.setBool("directionalLight.castShadows", false);
    }

    shader.setFloat("heightScale", 0.0f);
    const int MAX_POINT_LIGHTS = 10;
    for (int i = 0; i < std::min(static_cast<int>(pointLights.size()), MAX_POINT_LIGHTS); ++i) {
        std::string baseName = "pointLights[" + std::to_string(i) + "]";
        shader.setVec3(baseName + ".position", pointLights[i].position);
        shader.setVec3(baseName + ".color", pointLights[i].color);
        shader.setFloat(baseName + ".intensity", pointLights[i].intensity);
        shader.setFloat(baseName + ".radius", pointLights[i].radius);
    }
    shader.setInt("pointLightCount", std::min(static_cast<int>(pointLights.size()), MAX_POINT_LIGHTS));

    shader.setVec3("camPos", camera->Position);

    for (auto& [transform, meshComponent, materialComponent] : renderables) {
        auto material = materialComponent->material;
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

        shader.setMat4("model", transform->GetMatrix());
        shader.setMat4("view", camera->GetViewMatrix());
        shader.setMat4("projection", camera->projection);

        meshComponent->VAO.Bind();
        glDrawElements(GL_TRIANGLES, meshComponent->indexCount, GL_UNSIGNED_INT, 0);

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOG_ERROR << "OpenGL Error: " << err;
        }
    }

    renderables.clear();
}
