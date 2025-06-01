#include "rvelapch.h"
#include "LightingPass.h"
#include "Core/Utils/TextureManager.h"
#include "Core/Utils/MaterialManager.h"
#include "../../RvelaLog.h"

void LightingPass::Init()
{
    // MSAA framebuffer
    glGenFramebuffers(1, &o_ScreenFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, o_ScreenFBO);

    glGenTextures(1, &screenColorTex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, screenColorTex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA16F, ctx.viewportWidth, ctx.viewportHeight, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, screenColorTex, 0);

    glGenRenderbuffers(1, &screenRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, screenRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, ctx.viewportWidth, ctx.viewportHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screenRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "MSAA Framebuffer not complete" << std::endl;

    // Intermediate framebuffer (non-MSAA)
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

    glGenTextures(1, &o_IntermediateColorTex);
    glBindTexture(GL_TEXTURE_2D, o_IntermediateColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ctx.viewportWidth, ctx.viewportHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_IntermediateColorTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Intermediate Framebuffer not complete" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

LightingPass::~LightingPass()
{
    //TODO: Fill this function
}

void LightingPass::Execute()
{
    if (commands.empty() || !ctx.IsValid()) return;

    glBindFramebuffer(GL_FRAMEBUFFER, o_ScreenFBO);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    Shader& shader = Renderer::GetPBRShader();
    shader.use();

    bool hasDirLight = ctx.directionalLight.has_value();
    shader.setBool("hasDirectionalLight", hasDirLight);
    if (hasDirLight) {
        const auto& dirLight = ctx.directionalLight.value();
        shader.setVec3("directionalLight.direction", dirLight.direction);
        shader.setVec3("directionalLight.color", dirLight.color);
        shader.setFloat("directionalLight.intensity", dirLight.intensity);
        shader.setBool("directionalLight.castShadows", dirLight.castShadows);
        shader.setFloat("directionalLight.shadowBias", dirLight.shadowBias);
        shader.setFloat("directionalLight.blurRadius", dirLight.blurRadius);

        shader.setMat4("lightSpaceMatrix", i_LightSpaceMatrix);

        shader.setInt("shadowMap", 6);
        glBindTextureUnit(6, i_DirectionalShadowMap);

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
        shader.setFloat(base + ".blurRadius", light.blurRadius);
        shader.setFloat(base + ".shadowBias", light.shadowBias);
        shader.setBool(base + ".castShadows", light.castShadows);
    }

    constexpr int POINT_SHADOW_MAP_SLOT = 7;
    shader.setInt("pointShadowMap", POINT_SHADOW_MAP_SLOT);
    glBindTextureUnit(POINT_SHADOW_MAP_SLOT, i_PointShadowMap);

    shader.setInt("pointLightCount", pointLightCount);
    shader.setVec3("camPos", ctx.camera->Position);
    shader.setMat4("view", ctx.camera->GetViewMatrix());
    shader.setMat4("projection", ctx.camera->projection);

    for (auto& command : commands) {
        if (!ctx.camera->Intersects(command.mesh.worldAABB)) continue;

        auto& material = command.material.material;
        if (!material) continue;

        shader.setVec2("UVScale", material->UVScale);
        shader.setVec2("UVOffset", material->UVOffset);
        shader.setVec3("albedoColor", material->albedoColor);
        shader.setFloat("metallicValue", material->metallic);
        shader.setFloat("roughnessValue", material->roughness);
        shader.setFloat("aoValue", material->ao);
        shader.setFloat("normalScale", material->normalScale);

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
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(command.transform.GetWorldMatrix())));
        shader.setMat3("normalMatrix", normalMatrix);
        
        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);

        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            LOG_ERROR << "OpenGL Error: " << err;
        }

    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, o_ScreenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glBlitFramebuffer(
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    commands.clear();
}