#include "rvelapch.h"
#include "TransparentPass.h"
#include "Core/Log.h"
#include "Asset/AssetManager.h"
#include "Scene/Components.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"

using namespace rv;

struct MapInfo {
    bool isUsing;
    std::string uniformName;
    int slot;
    std::string useUniform;
    Ref<TextureAsset> texture;
};

void TransparentPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
}

TransparentPass::~TransparentPass()
{
}

void TransparentPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.transparentCommands;
    auto& resourceRegistry = frame.registry;

    auto screenFBO = resourceRegistry.Get("ScreenBuffer")->id;
    auto intermediateFBO = resourceRegistry.Get("IntermediateBuffer")->id;

    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    if (!commands.empty())
    {
        auto i_DirectionalShadowMap = resourceRegistry.Get("DirectionalShadowMap")->id;
        auto i_PointShadowMap = resourceRegistry.Get("PointShadowMap")->id;

        Shader& shader = ShaderManager::Get("PBR");
        shader.use();

        constexpr int DIR_SHADOW_MAP_SLOT = 6;
        constexpr int POINT_SHADOW_MAP_SLOT = 7;

        shader.setInt("shadowMap", DIR_SHADOW_MAP_SLOT);
        glBindTextureUnit(DIR_SHADOW_MAP_SLOT, i_DirectionalShadowMap);

        shader.setInt("pointShadowMap", POINT_SHADOW_MAP_SLOT);
        glBindTextureUnit(POINT_SHADOW_MAP_SLOT, i_PointShadowMap);

        auto& env = *ctx.environment;
        auto& skybox = env.GetSkybox();

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

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);

        auto ApplyCullMode = [](CullMode mode) {
            if (mode == CullMode::Disabled) {
                glDisable(GL_CULL_FACE);
            }
            else {
                glEnable(GL_CULL_FACE);
                glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
            }
            };

        auto ApplyBlendMode = [](BlendMode mode) {
            switch (mode) {
            case BlendMode::Mix:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glBlendEquation(GL_FUNC_ADD);
                break;
            case BlendMode::Add:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glBlendEquation(GL_FUNC_ADD);
                break;
            case BlendMode::Subtract:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                break;
            case BlendMode::Multiply:
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                glBlendEquation(GL_FUNC_ADD);
                break;
            }
            };

        for (auto& command : commands) {
            if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

            auto* material = command.material;

            ApplyCullMode(material->GetCullMode());
            ApplyBlendMode(material->GetBlendMode());

            shader.setInt("shadingMode", static_cast<int>(material->GetShadingMode()));
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

            shader.setMat4("model", command.transform->GetWorldMatrix());
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(command.transform->GetWorldMatrix())));
            shader.setMat3("normalMatrix", normalMatrix);

            command.mesh->VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

            for (const auto& map : maps)
            {
                glBindSampler(map.slot, 0);
            }
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glBlitFramebuffer(
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
}