#include "rvelapch.h"
#include "LightingPass.h"
#include "Core/Log.h"
#include "Asset/AssetUUID.h"
#include "Asset/AssetManager.h"
#include "Scene/Components.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"
#include <glm/gtc/type_ptr.hpp>

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
    FramebufferDesc screenDesc;
    screenDesc.width = ctx.viewportWidth;
    screenDesc.height = ctx.viewportHeight;
    screenDesc.samples = 2;
    screenDesc.colorAttachments = {
        { FramebufferTextureFormat::RGBA16F }
    };
    screenDesc.hasDepth = true;
    screenDesc.depthAttachment = { FramebufferTextureFormat::Depth24Stencil8 };

    m_ScreenFramebuffer = Framebuffer(screenDesc);

    // Intermediate framebuffer
    FramebufferDesc intermediateDesc;
    intermediateDesc.width = ctx.viewportWidth;
    intermediateDesc.height = ctx.viewportHeight;
    intermediateDesc.colorAttachments = {
        { FramebufferTextureFormat::RGBA16F, FramebufferFilterMode::Linear }
    };
    intermediateDesc.hasDepth = false;

    m_IntermediateFramebuffer = Framebuffer(intermediateDesc);

    frame.registry.Register("ScreenTexture", { RenderResourceType::Texture, m_IntermediateFramebuffer.GetColorAttachment(0) });
    frame.registry.Register("ScreenBuffer", { RenderResourceType::Framebuffer, m_ScreenFramebuffer.GetID() });
    frame.registry.Register("IntermediateBuffer", { RenderResourceType::Framebuffer, m_IntermediateFramebuffer.GetID() });
}

void LightingPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& staticCommands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;
    auto& resourceRegistry = frame.registry;

    auto i_DirectionalShadowMap = resourceRegistry.Get("DirectionalShadowMap")->id;
    auto i_PointShadowMap = resourceRegistry.Get("PointShadowMap")->id;
    auto i_SSAO = resourceRegistry.Get("SSAOTexture")->id;

    m_ScreenFramebuffer.BindViewport();

    glDepthMask(GL_TRUE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    constexpr int DIR_SHADOW_MAP_SLOT = 6;
    constexpr int POINT_SHADOW_MAP_SLOT = 7;
    constexpr int SSAO_SLOT = 11;

    auto& env = *ctx.environment;
    auto& skybox = env.GetSkybox();

    auto BindGlobalTexturesAndUniforms = [&](Shader& shader) {
        shader.use();

        shader.setInt("shadowMap", DIR_SHADOW_MAP_SLOT);
        glBindTextureUnit(DIR_SHADOW_MAP_SLOT, i_DirectionalShadowMap);

        shader.setInt("pointShadowMap", POINT_SHADOW_MAP_SLOT);
        glBindTextureUnit(POINT_SHADOW_MAP_SLOT, i_PointShadowMap);

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

        shader.setBool("useSSAO", ctx.environment->SSAO);
        shader.setInt("ssaoTexture", SSAO_SLOT);
        glBindTextureUnit(SSAO_SLOT, i_SSAO);
        };

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
        }
        };

    auto SetupMaterial = [](Shader& shader, MaterialComponent* material) {
        shader.setInt("shadingMode", static_cast<int>(material->GetShadingMode()));
        shader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
        shader.setBool("receiveShadows", material->GetReceiveShadows());
        shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        shader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());
        shader.setVec4("albedoColor", material->GetAlbedoColor());
        shader.setVec3("emmisiveColor", material->GetEmissiveColor());
        shader.setFloat("emmisiveIntensity", material->GetEmissiveIntensity());
        shader.setFloat("metallicValue", material->GetMetallic());
        shader.setFloat("roughnessValue", material->GetRoughness());
        shader.setFloat("aoValue", material->GetAO());
        shader.setFloat("normalScale", material->GetNormalScale());
        shader.setFloat("specularIntensity", material->GetSpecular());
        shader.setFloat("heightScale", material->GetHeightScale());

        std::array<MapInfo, 6> maps = { {
            { material->IsUsingAlbedoMap(),    "albedoMap",    0, "useAlbedoMap",    material->GetAlbedoTexture() },
            { material->IsUsingNormalMap(),    "normalMap",    1, "useNormalMap",    material->GetNormalTexture() },
            { material->IsUsingMetallicMap(),  "metallicMap",  2, "useMetallicMap",  material->GetMetallicTexture() },
            { material->IsUsingRoughnessMap(), "roughnessMap", 3, "useRoughnessMap", material->GetRoughnessTexture() },
            { material->IsUsingAOMap(),        "aoMap",        4, "useAOMap",        material->GetAOTexture() },
            { material->IsUsingHeightMap(),    "heightMap",    5, "useHeightMap",    material->GetHeightTexture() }
        } };

        for (const auto& map : maps)
        {
            shader.setBool(map.useUniform, map.isUsing);
            if (map.isUsing && map.texture)
            {
                shader.setInt(map.uniformName, map.slot);
                TextureCache::Get().GetOrCreate(map.texture).Bind(map.slot);
                material->GetSampler().Bind(map.slot);
            }
            else
            {
                glBindSampler(map.slot, 0);
                glBindTextureUnit(map.slot, 0);
            }
        }
        };

    auto CleanupMaterialSamplers = []() {
        for (int slot = 0; slot < 6; ++slot)
            glBindSampler(slot, 0);
        };

    if (!staticCommands.empty())
    {
        Shader& staticShader = ShaderManager::Get("PBR");
        BindGlobalTexturesAndUniforms(staticShader);

        for (auto& command : staticCommands)
        {
            if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

            glDisable(GL_STENCIL_TEST);
            ApplyCullMode(command.material->GetCullMode());

            SetupMaterial(staticShader, command.material);

            staticShader.setMat4("model", command.transform->GetWorldMatrix());
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(command.transform->GetWorldMatrix())));
            staticShader.setMat3("normalMatrix", normalMatrix);

            command.mesh->VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

            CleanupMaterialSamplers();
        }
    }

    if (!skeletalCommands.empty())
    {
        Shader& skeletalShader = ShaderManager::Get("PBR_Skeletal");
        BindGlobalTexturesAndUniforms(skeletalShader);

        GLint boneLoc = glGetUniformLocation(skeletalShader.ID, "u_BoneMatrices");

        for (auto& command : skeletalCommands)
        {
            if (!command.mesh || command.mesh->indexCount == 0) continue;
            //if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue; TODO: sswwwwwwwwwwwwwwwwwwwww

            glDisable(GL_STENCIL_TEST);
            ApplyCullMode(command.material->GetCullMode());

            SetupMaterial(skeletalShader, command.material);

            skeletalShader.setMat4("model", command.transform->GetWorldMatrix());
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(command.transform->GetWorldMatrix())));
            skeletalShader.setMat3("normalMatrix", normalMatrix);

            if (boneLoc != -1 && command.skeleton && !command.skeleton->skinningPalette.empty())
            {
                glUniformMatrix4fv(
                    boneLoc,
                    static_cast<GLsizei>(command.skeleton->skinningPalette.size()),
                    GL_FALSE,
                    glm::value_ptr(command.skeleton->skinningPalette[0])
                );
            }

            command.mesh->VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

            CleanupMaterialSamplers();
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}