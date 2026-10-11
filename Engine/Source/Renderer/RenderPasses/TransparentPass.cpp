#include "rvelapch.h"
#include "TransparentPass.h"
#include "Core/Log.h"
#include "Asset/AssetManager.h"
#include "Scene/Components.h"
#include "Renderer/RenderContext.h"
#include "Scene/Environment.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"
#include "Core/Time.h"
#include <glm/gtc/type_ptr.hpp>

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
    auto& skeletalCommands = frame.skeletalTransparentCommands;
    auto& resourceRegistry = frame.registry;

    auto screenFBO = resourceRegistry.Get("ScreenBuffer")->id;
    auto intermediateFBO = resourceRegistry.Get("IntermediateBuffer")->id;

    // Blit opaque scene to intermediateFBO so transparent shaders can sample ScreenTexture
    glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glBlitFramebuffer(
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        0, 0, ctx.viewportWidth, ctx.viewportHeight,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    if (!commands.empty() || !skeletalCommands.empty())
    {
        auto i_DirectionalShadowMap = resourceRegistry.Get("DirectionalShadowMap")->id;
        auto i_PointShadowMap = resourceRegistry.Get("PointShadowMap")->id;

        constexpr int DIR_SHADOW_MAP_SLOT = 6;
        constexpr int POINT_SHADOW_MAP_SLOT = 7;
        constexpr int ARM_MAP_SLOT = 12;

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

            auto* screenRes = resourceRegistry.Get("ScreenTexture");
            if (screenRes && screenRes->id != 0)
            {
                shader.setInt("u_EngineScreenTexture", 14);
                glBindTextureUnit(14, screenRes->id);
            }

            auto* depthRes = resourceRegistry.Get("DepthTexture");
            if (depthRes && depthRes->id != 0)
            {
                shader.setInt("u_EngineDepthTexture", 15);
                glBindTextureUnit(15, depthRes->id);
            }

            auto* normalRes = resourceRegistry.Get("NormalTexture");
            if (normalRes && normalRes->id != 0)
            {
                shader.setInt("u_EngineNormalTexture", 16);
                glBindTextureUnit(16, normalRes->id);
            }
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

        auto SetupMaterial = [](Shader& shader, MaterialComponent* material) {
            shader.setInt("shadingMode", static_cast<int>(material->GetShadingMode()));
            shader.setBool("receiveShadows", material->GetReceiveShadows());
            shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
            shader.setFloat("alphaCutoff", material->GetAlphaCutoff());
            shader.setVec2("UVScale", material->GetUVScale());
            shader.setVec2("UVOffset", material->GetUVOffset());
            shader.setBool("useTriplanar", material->GetUseTriplanar());
            shader.setBool("useWorldTriplanar", material->GetUseWorldTriplanar());
            shader.setFloat("triplanarSharpness", material->GetTriplanarSharpness());
            shader.setVec4("albedoColor", material->GetAlbedoColor());
            shader.setVec3("emmisiveColor", material->GetEmissiveColor());
            shader.setFloat("emmisiveIntensity", material->GetEmissiveIntensity());
            shader.setFloat("metallicValue", material->GetMetallic());
            shader.setFloat("roughnessValue", material->GetRoughness());
            shader.setFloat("aoValue", material->GetAO());
            shader.setFloat("normalScale", material->GetNormalScale());
            shader.setFloat("specularIntensity", material->GetSpecular());
            shader.setFloat("heightScale", material->GetHeightScale());

            std::array<MapInfo, 7> maps = { {
                { material->IsUsingAlbedoMap(),    "albedoMap",    0, "useAlbedoMap",    material->GetAlbedoTexture() },
                { material->IsUsingNormalMap(),    "normalMap",    1, "useNormalMap",    material->GetNormalTexture() },
                { material->IsUsingMetallicMap(),  "metallicMap",  2, "useMetallicMap",  material->GetMetallicTexture() },
                { material->IsUsingRoughnessMap(), "roughnessMap", 3, "useRoughnessMap", material->GetRoughnessTexture() },
                { material->IsUsingAOMap(),        "aoMap",        4, "useAOMap",        material->GetAOTexture() },
                { material->IsUsingHeightMap(),    "heightMap",    5, "useHeightMap",    material->GetHeightTexture() },
                { material->IsUsingARMMap(),       "armMap",       ARM_MAP_SLOT, "useARMMap", material->GetARMTexture() }
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
            glBindSampler(ARM_MAP_SLOT, 0);
            for (int slot = 13; slot < 28; ++slot)
            {
                glBindSampler(slot, 0);
                glBindTextureUnit(slot, 0);
            }
            };

        auto BindCustomShaderUniforms = [](Shader& shader, const MaterialInstance& inst, const ShaderAsset& shaderAsset) {
            int nextTextureSlot = 17;
            const auto& overrides = inst.GetCustomUniformOverrides();
            const auto& cached = inst.GetCachedCustomUniforms();

            for (const auto& uDef : shaderAsset.uniforms)
            {
                if (uDef.engineTexture != EngineTextureType::None)
                {
                    if (uDef.hasCustomSampler)
                    {
                        auto itSampler = shaderAsset.samplers.find(uDef.name);
                        if (itSampler != shaderAsset.samplers.end() && itSampler->second)
                        {
                            if (uDef.engineTexture == EngineTextureType::Screen)
                                itSampler->second->Bind(14);
                            else if (uDef.engineTexture == EngineTextureType::Depth)
                                itSampler->second->Bind(15);
                            else if (uDef.engineTexture == EngineTextureType::Normal)
                                itSampler->second->Bind(16);
                        }
                    }
                    continue;
                }

                CustomUniformValue val;
                auto itOver = overrides.find(uDef.name);
                if (itOver != overrides.end())
                {
                    val = itOver->second;
                }
                else
                {
                    auto itCached = cached.find(uDef.name);
                    if (itCached != cached.end())
                        val = itCached->second;
                    else
                    {
                        val.type = uDef.type;
                        val.floatVal = uDef.floatVal;
                        val.intVal = uDef.intVal;
                        val.boolVal = uDef.boolVal;
                        val.vec2Val = uDef.vec2Val;
                        val.vec3Val = uDef.vec3Val;
                        val.vec4Val = uDef.vec4Val;
                        val.textureUUID = uDef.textureUUID;
                    }
                }

                switch (uDef.type)
                {
                case ShaderUniformType::Float:
                    shader.setFloat(uDef.name, val.floatVal);
                    break;
                case ShaderUniformType::Int:
                    shader.setInt(uDef.name, val.intVal);
                    break;
                case ShaderUniformType::Bool:
                    shader.setBool(uDef.name, val.boolVal);
                    break;
                case ShaderUniformType::Vec2:
                    shader.setVec2(uDef.name, val.vec2Val);
                    break;
                case ShaderUniformType::Vec3:
                case ShaderUniformType::Color3:
                    shader.setVec3(uDef.name, val.vec3Val);
                    break;
                case ShaderUniformType::Vec4:
                case ShaderUniformType::Color4:
                    shader.setVec4(uDef.name, val.vec4Val);
                    break;
                case ShaderUniformType::Texture2D:
                    shader.setInt(uDef.name, nextTextureSlot);
                    if (val.textureUUID.IsValid())
                    {
                        auto texAsset = AssetManager::Get().GetAsset<TextureAsset>(val.textureUUID);
                        if (texAsset)
                        {
                            TextureCache::Get().GetOrCreate(texAsset).Bind(nextTextureSlot);
                        }
                        else
                        {
                            TextureCache::Get().GetWhiteTexture().Bind(nextTextureSlot);
                        }
                    }
                    else
                    {
                        TextureCache::Get().GetWhiteTexture().Bind(nextTextureSlot);
                    }

                    if (uDef.hasCustomSampler)
                    {
                        auto itSampler = shaderAsset.samplers.find(uDef.name);
                        if (itSampler != shaderAsset.samplers.end() && itSampler->second)
                        {
                            itSampler->second->Bind(nextTextureSlot);
                        }
                        else
                        {
                            inst.GetSampler().Bind(nextTextureSlot);
                        }
                    }
                    else
                    {
                        inst.GetSampler().Bind(nextTextureSlot);
                    }
                    nextTextureSlot++;
                    break;
                }
            }
            };

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);

        auto UpdateScreenTexture = [&]() {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
            glBlitFramebuffer(
                0, 0, ctx.viewportWidth, ctx.viewportHeight,
                0, 0, ctx.viewportWidth, ctx.viewportHeight,
                GL_COLOR_BUFFER_BIT, GL_NEAREST
            );
            glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
            glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
        };

        if (!commands.empty())
        {
            Shader& staticShader = ShaderManager::Get("PBR");
            BindGlobalTexturesAndUniforms(staticShader);
            Shader* lastBoundShader = &staticShader;

            if (ctx.wireframe)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            for (auto& command : commands) {
                if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

                auto* material = command.material;
                ApplyCullMode(material->GetCullMode());
                ApplyBlendMode(material->GetBlendMode());

                Shader* currentShader = &staticShader;
                auto customShaderAsset = material->GetInstance().GetCustomShaderAsset();
                bool hasCustomShader = (customShaderAsset && customShaderAsset->IsValid() && customShaderAsset->staticShader);

                if (hasCustomShader)
                {
                    bool usesScreenTexture = false;
                    for (const auto& uDef : customShaderAsset->uniforms) {
                        if (uDef.engineTexture == EngineTextureType::Screen) {
                            usesScreenTexture = true;
                            break;
                        }
                    }
                    if (!usesScreenTexture && customShaderAsset->source.find("SCREEN_TEXTURE") != std::string::npos) {
                        usesScreenTexture = true;
                    }

                    if (usesScreenTexture) {
                        UpdateScreenTexture();
                    }

                    if (customShaderAsset->renderModes.hasCullMode)
                        ApplyCullMode(customShaderAsset->renderModes.cullMode);
                    if (customShaderAsset->renderModes.hasBlendMode)
                        ApplyBlendMode(customShaderAsset->renderModes.blendMode);
                    if (customShaderAsset->renderModes.depthTestDisabled)
                        glDisable(GL_DEPTH_TEST);
                    if (customShaderAsset->renderModes.depthDrawAlways)
                        glDepthMask(GL_TRUE);
                    if (customShaderAsset->renderModes.wireframe)
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                    currentShader = customShaderAsset->staticShader.get();
                    if (usesScreenTexture || currentShader != lastBoundShader)
                    {
                        currentShader->use();
                        BindGlobalTexturesAndUniforms(*currentShader);
                        lastBoundShader = currentShader;
                    }
                    if (customShaderAsset->isLit)
                    {
                        currentShader->setBool("receiveShadows", material->GetReceiveShadows());
                    }
                    BindCustomShaderUniforms(*currentShader, material->GetInstance(), *customShaderAsset);
                }
                else
                {
                    if (lastBoundShader != &staticShader)
                    {
                        staticShader.use();
                        BindGlobalTexturesAndUniforms(staticShader);
                        lastBoundShader = &staticShader;
                    }
                    SetupMaterial(staticShader, material);
                }

                currentShader->setMat4("model", command.transform->GetWorldMatrix());
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(command.transform->GetWorldMatrix())));
                currentShader->setMat3("normalMatrix", normalMatrix);
                currentShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                currentShader->setVec2("UVScale", material->GetUVScale());
                currentShader->setVec2("UVOffset", material->GetUVOffset());

                command.mesh->VAO.Bind();
                glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                if (hasCustomShader)
                {
                    if (customShaderAsset->renderModes.wireframe || ctx.wireframe)
                        glPolygonMode(GL_FRONT_AND_BACK, ctx.wireframe ? GL_LINE : GL_FILL);
                    if (customShaderAsset->renderModes.depthTestDisabled)
                        glEnable(GL_DEPTH_TEST);
                    if (customShaderAsset->renderModes.depthDrawAlways)
                        glDepthMask(GL_FALSE);
                    if (customShaderAsset->renderModes.hasCullMode)
                        ApplyCullMode(material->GetCullMode());
                    if (customShaderAsset->renderModes.hasBlendMode)
                        ApplyBlendMode(material->GetBlendMode());
                }

                CleanupMaterialSamplers();
            }
        }

        if (!skeletalCommands.empty())
        {
            Shader& skeletalShader = ShaderManager::Get("PBR_Skeletal");
            BindGlobalTexturesAndUniforms(skeletalShader);
            Shader* lastBoundShader = &skeletalShader;

            GLint defaultBoneLoc = glGetUniformLocation(skeletalShader.ID, "u_BoneMatrices");

            for (auto& command : skeletalCommands) {
                if (!command.mesh || command.mesh->indexCount == 0) continue;

                auto* material = command.material;
                ApplyCullMode(material->GetCullMode());
                ApplyBlendMode(material->GetBlendMode());

                Shader* currentShader = &skeletalShader;
                auto customShaderAsset = material->GetInstance().GetCustomShaderAsset();
                bool hasCustomShader = (customShaderAsset && customShaderAsset->IsValid() && customShaderAsset->skeletalShader);

                if (hasCustomShader)
                {
                    bool usesScreenTexture = false;
                    for (const auto& uDef : customShaderAsset->uniforms) {
                        if (uDef.engineTexture == EngineTextureType::Screen) {
                            usesScreenTexture = true;
                            break;
                        }
                    }
                    if (!usesScreenTexture && customShaderAsset->source.find("SCREEN_TEXTURE") != std::string::npos) {
                        usesScreenTexture = true;
                    }

                    if (usesScreenTexture) {
                        UpdateScreenTexture();
                    }

                    if (customShaderAsset->renderModes.hasCullMode)
                        ApplyCullMode(customShaderAsset->renderModes.cullMode);
                    if (customShaderAsset->renderModes.hasBlendMode)
                        ApplyBlendMode(customShaderAsset->renderModes.blendMode);
                    if (customShaderAsset->renderModes.depthTestDisabled)
                        glDisable(GL_DEPTH_TEST);
                    if (customShaderAsset->renderModes.depthDrawAlways)
                        glDepthMask(GL_TRUE);
                    if (customShaderAsset->renderModes.wireframe)
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                    currentShader = customShaderAsset->skeletalShader.get();
                    if (usesScreenTexture || currentShader != lastBoundShader)
                    {
                        currentShader->use();
                        BindGlobalTexturesAndUniforms(*currentShader);
                        lastBoundShader = currentShader;
                    }
                    if (customShaderAsset->isLit)
                    {
                        currentShader->setBool("receiveShadows", material->GetReceiveShadows());
                    }
                    BindCustomShaderUniforms(*currentShader, material->GetInstance(), *customShaderAsset);
                }
                else
                {
                    if (lastBoundShader != &skeletalShader)
                    {
                        skeletalShader.use();
                        BindGlobalTexturesAndUniforms(skeletalShader);
                        lastBoundShader = &skeletalShader;
                    }
                    SetupMaterial(skeletalShader, material);
                }

                currentShader->setMat4("model", command.transform->GetWorldMatrix());
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(command.transform->GetWorldMatrix())));
                currentShader->setMat3("normalMatrix", normalMatrix);
                currentShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                currentShader->setVec2("UVScale", material->GetUVScale());
                currentShader->setVec2("UVOffset", material->GetUVOffset());

                GLint boneLoc = hasCustomShader ? glGetUniformLocation(currentShader->ID, "u_BoneMatrices") : defaultBoneLoc;
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

                if (hasCustomShader)
                {
                    if (customShaderAsset->renderModes.wireframe || ctx.wireframe)
                        glPolygonMode(GL_FRONT_AND_BACK, ctx.wireframe ? GL_LINE : GL_FILL);
                    if (customShaderAsset->renderModes.depthTestDisabled)
                        glEnable(GL_DEPTH_TEST);
                    if (customShaderAsset->renderModes.depthDrawAlways)
                        glDepthMask(GL_FALSE);
                    if (customShaderAsset->renderModes.hasCullMode)
                        ApplyCullMode(material->GetCullMode());
                    if (customShaderAsset->renderModes.hasBlendMode)
                        ApplyBlendMode(material->GetBlendMode());
                }

                CleanupMaterialSamplers();
            }
        }

        if (ctx.wireframe)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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