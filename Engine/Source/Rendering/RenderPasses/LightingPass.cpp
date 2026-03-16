#include "rvelapch.h"
#include "LightingPass.h"
#include "Core/Log.h"
#include "Asset/AssetUUID.h"
#include "Asset/AssetManager.h"
#include "Scene/Components.h"
#include "Rendering/RenderContext.h"
#include "Scene/Environment.h"
#include "Rendering/ShaderManager.h"
#include "Rendering/TextureCache.h"

using namespace rv;

struct MapInfo {
    bool isUsing;
    std::string uniformName;
    int slot;
    std::string useUniform;
    Ref<TextureAsset> texture;
};

void LightingPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    // MSAA framebuffer
    glGenFramebuffers(1, &o_ScreenFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, o_ScreenFBO);

    glGenTextures(1, &screenColorTex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, screenColorTex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_RGBA16F, ctx.viewportWidth, ctx.viewportHeight, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, screenColorTex, 0);

    glGenRenderbuffers(1, &screenRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, screenRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_DEPTH24_STENCIL8, ctx.viewportWidth, ctx.viewportHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screenRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LOG_ERROR("MSAA Framebuffer not complete");

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
        LOG_ERROR("Intermediate Framebuffer not complete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("ScreenTexture", { RenderResourceType::Texture,o_IntermediateColorTex });
    frame.registry.Register("ScreenBuffer", { RenderResourceType::Framebuffer,o_ScreenFBO });
    frame.registry.Register("IntermediateBuffer", { RenderResourceType::Framebuffer,intermediateFBO });
}


LightingPass::~LightingPass()
{
    //TODO: Fill this function
}

void LightingPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.commands;
    auto& resourceRegistry = frame.registry;

  

    auto i_DirectionalShadowMap = resourceRegistry.Get("DirectionalShadowMap")->id;
    auto i_PointShadowMap = resourceRegistry.Get("PointShadowMap")->id;

    glBindFramebuffer(GL_FRAMEBUFFER, o_ScreenFBO);

    glClear(GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    Shader& shader = ShaderManager::Get("PBR");
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

        shader.setMat4("lightSpaceMatrix", ctx.directionalLight->lightSpace);

        shader.setInt("shadowMap", 6);
        glBindTextureUnit(6, i_DirectionalShadowMap);

       
    }


    GLint maxTextureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);

    static constexpr int RESERVED_SLOTS = 7;
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
    shader.setMat4("projection", ctx.camera->GetProjectionMatrix());



    auto& env = *ctx.environment;
    auto& skybox = env.GetSkybox();

    glBindTextureUnit(6, i_DirectionalShadowMap);
    glBindTextureUnit(7, i_PointShadowMap);

    shader.setInt("irradianceMap", 8);
    shader.setInt("prefilterMap", 9);
    shader.setInt("brdfLUT", 10);
    shader.setFloat("iblIntensity", env.Lighting_IBLIntensity);
    shader.setBool("useIBL", env.Lighting_IBL);
    shader.setVec3("ambientColor", env.Lighting_AmbientColor);
    shader.setFloat("ambientIntensity", env.Lighting_AmbientIntensity);
        
    glBindTextureUnit(8, skybox.GetIrradianceMap());
    glBindTextureUnit(9, skybox.GetPrefilterMap());
    glBindTextureUnit(10, skybox.GetBRDFLUTTexture());

    size_t drawCallCounter = 0;
    for (auto& command : commands) {
        if (!ctx.camera->Intersects(command.mesh.worldAABB)) continue;
        
         glDisable(GL_STENCIL_TEST);

        if (!command.mesh.IsDoubleSided())
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }

        auto& material = command.material;

        shader.setVec2("UVScale", material.GetUVScale());
        shader.setVec2("UVOffset", material.GetUVOffset());
        shader.setVec3("albedoColor", material.GetAlbedoColor());
        shader.setVec3("emmisiveColor", material.GetEmissiveColor()); // shader typo 
        shader.setFloat("emmisiveIntensity", material.GetEmissiveIntensity()); // shader typo
        shader.setFloat("metallicValue", material.GetMetallic());
        shader.setFloat("roughnessValue", material.GetRoughness());
        shader.setFloat("aoValue", material.GetAO());
        shader.setFloat("normalScale", material.GetNormalScale());
        shader.setFloat("specularIntensity", material.GetSpecular());
        shader.setFloat("heightScale", material.GetHeightScale());

        std::array<MapInfo, 6> maps = { {
        { material.IsUsingAlbedoMap(),    "albedoMap",    0, "useAlbedoMap",    material.GetAlbedoTexture() },
        { material.IsUsingNormalMap(),    "normalMap",    1, "useNormalMap",    material.GetNormalTexture() },
        { material.IsUsingMetallicMap(),  "metallicMap",  2, "useMetallicMap",  material.GetMetallicTexture() },
        { material.IsUsingRoughnessMap(), "roughnessMap", 3, "useRoughnessMap", material.GetRoughnessTexture() },
        { material.IsUsingAOMap(),        "aoMap",        4, "useAOMap",        material.GetAOTexture() },
        { material.IsUsingHeightMap(),    "heightMap",    5, "useHeightMap",    material.GetHeightTexture() }
        } };

        for (const auto& map : maps) 
        {
            shader.setBool(map.useUniform, map.isUsing);
            if (map.isUsing)
            {
                shader.setInt(map.uniformName, map.slot);
                TextureCache::Get().GetOrCreate(map.texture).Bind(map.slot);
                material.GetSampler().Bind(map.slot);
            }
        }

        shader.setMat4("model", command.transform.GetWorldMatrix());
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(command.transform.GetWorldMatrix())));
        shader.setMat3("normalMatrix", normalMatrix);
        
        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
        drawCallCounter++;
        for (const auto& map : maps)
        {
            glBindSampler(map.slot, 0); // TODO: Material sorting
        }
    }
    //LOG_DEBUG("Draw calls: {}", drawCallCounter);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, o_ScreenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glBlitFramebuffer(
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
   
    resourceRegistry.Register("ScreenTexture", { RenderResourceType::Texture,o_IntermediateColorTex });
    resourceRegistry.Register("ScreenBuffer", { RenderResourceType::Framebuffer,o_ScreenFBO });
    resourceRegistry.Register("IntermediateBuffer", { RenderResourceType::Framebuffer,intermediateFBO });
}

