#include "rvelapch.h"
#include "MeshPass.h"
#include "Core/Utils/TextureManager.h"
#include "Core/Utils/MaterialManager.h"
#include "../../RvelaLog.h"

bool MeshPass::isInitialized = false;
GLuint MeshPass::screenFBO = 0; 
GLuint MeshPass::screenColorTex = 0;
GLuint MeshPass::screenRBO = 0;
GLuint MeshPass::intermediateFBO = 0;
GLuint MeshPass::intermediateColorTex = 0;

void MeshPass::Execute() {
    if (commands.empty() || !ctx.IsValid()) return;

    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shader& shader = Renderer::GetDefaultShader();
    shader.use();

    bool hasDirLight = ctx.directionalLight.has_value();
    shader.setBool("hasDirectionalLight", hasDirLight);
    if (hasDirLight) {
        const auto& dirLight = ctx.directionalLight.value();
        shader.setVec3("directionalLight.direction", dirLight.direction);
        shader.setVec3("directionalLight.color", dirLight.color);
        shader.setFloat("directionalLight.intensity", dirLight.intensity);
        shader.setBool("directionalLight.castShadows", dirLight.castShadows);
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        shader.setInt("shadowMap", 6);

        glActiveTexture(GL_TEXTURE0 + 6);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) LOG_ERROR << "OpenGL Error after directional light setup: " << err;
    }

    shader.setFloat("heightScale", 0.0f);

    GLint maxTextureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);

    constexpr int RESERVED_SLOTS = 7;
    int maxPointLights = std::max(0, maxTextureUnits - RESERVED_SLOTS);
    int pointLightCount = std::min(static_cast<int>(ctx.pointLights.size()), maxPointLights);

    for (int i = 0; i < pointLightCount; ++i) {
        const auto& light = ctx.pointLights[i];
        std::string base = "pointLights[" + std::to_string(i) + "]";
        shader.setVec3(base + ".position", light.position);
        shader.setVec3(base + ".color", light.color);
        shader.setFloat(base + ".intensity", light.intensity);
        shader.setFloat(base + ".radius", light.radius);
        shader.setFloat(base + ".falloff", light.falloff);
        shader.setInt(base + ".shadowIndex", light.shadowIndex);
        shader.setBool(base + ".castShadows", light.castShadows);
    }

    constexpr int POINT_SHADOW_MAP_SLOT = 10;
    shader.setInt("pointShadowMap", POINT_SHADOW_MAP_SLOT);
    glActiveTexture(GL_TEXTURE0 + POINT_SHADOW_MAP_SLOT);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, pointShadowMap);

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
    glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glBlitFramebuffer(
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    commands.clear();
}