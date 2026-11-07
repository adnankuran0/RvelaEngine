#include "rvelapch.h"
#include "ShadowPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderTypes.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"
#include "Utils/ProjectManager.h"
#include "Core/Time.h"
#include "Asset/Types/ShaderAsset.h"

using namespace rv;

void ShadowPass::InvalidateCache()
{
    m_CachedDirectionalShadow = {};
    for (auto& s : m_CachedPointShadows)
        s = {};
    for (auto& s : m_CachedSpotShadows)
        s = {};
}

void ShadowPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    InitDirectionalShadowMap();
    InitPointShadowMap();
    InitSpotShadowMap();

    frame.registry.Register("DirectionalShadowMap", { RenderResourceType::Texture, o_DirectionalShadowMap });
    frame.registry.Register("PointShadowMap", { RenderResourceType::Texture, o_PointShadowMap });
    frame.registry.Register("SpotShadowMap", { RenderResourceType::Texture, o_SpotShadowMap });
}

void ShadowPass::InitDirectionalShadowMap()
{
    glGenFramebuffers(1, &dirShadowFBO);

    glGenTextures(1, &o_DirectionalShadowMap);
    glBindTexture(GL_TEXTURE_2D_ARRAY, o_DirectionalShadowMap);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_DEPTH_COMPONENT24,
        m_CurrentShadowResolution,
        m_CurrentShadowResolution,
        NUM_SHADOW_CASCADES,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, dirShadowFBO);
    for (int cascade = 0; cascade < NUM_SHADOW_CASCADES; ++cascade) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_DirectionalShadowMap, 0, cascade);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_DirectionalShadowMap, 0, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Directional shadow framebuffer not complete! Status: {:#x}", status);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::InitPointShadowMap()
{
    glGenFramebuffers(1, &pointFBO);

    glGenTextures(1, &o_PointShadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, o_PointShadowMap);
    glTexImage3D(
        GL_TEXTURE_CUBE_MAP_ARRAY,
        0,
        GL_DEPTH_COMPONENT24,
        POINT_SHADOW_WIDTH,
        POINT_SHADOW_HEIGHT,
        6 * MaxPointLights,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    for (int layer = 0; layer < 6 * MaxPointLights; ++layer) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_PointShadowMap, 0, layer);
        glClear(GL_DEPTH_BUFFER_BIT);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Point shadow framebuffer layer {} not complete!", layer);
        }
    }
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::InitSpotShadowMap()
{
    glGenFramebuffers(1, &spotFBO);

    glGenTextures(1, &o_SpotShadowMap);
    glBindTexture(GL_TEXTURE_2D_ARRAY, o_SpotShadowMap);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_DEPTH_COMPONENT24,
        SPOT_SHADOW_WIDTH,
        SPOT_SHADOW_HEIGHT,
        MaxSpotLights,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, spotFBO);
    for (int layer = 0; layer < MaxSpotLights; ++layer) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_SpotShadowMap, 0, layer);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("Spot shadow framebuffer not complete! Status: {:#x}", status);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::RenderDirectionalShadowMap(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;

    if (!ctx.directionalLight || !ctx.directionalLight->castShadows) return;

    bool isDirty = true;
    if (ctx.directionalLight->staticShadow)
    {
        if (m_CachedDirectionalShadow.valid &&
            !ctx.directionalLight->shadowDirty &&
            m_CachedDirectionalShadow.lightID == ctx.directionalLight->lightID &&
            m_CachedDirectionalShadow.direction == ctx.directionalLight->direction &&
            m_CachedDirectionalShadow.shadowBias == ctx.directionalLight->shadowBias &&
            m_CachedDirectionalShadow.normalBias == ctx.directionalLight->normalBias &&
            m_CachedDirectionalShadow.reverseCullFace == ctx.directionalLight->reverseCullFace)
        {
            bool cascadesMatch = true;
            for (int i = 0; i < NUM_SHADOW_CASCADES; ++i)
            {
                if (m_CachedDirectionalShadow.cascadeMatrices[i] != ctx.directionalLight->cascadeMatrices[i])
                {
                    cascadesMatch = false;
                    break;
                }
            }
            if (cascadesMatch)
                isDirty = false;
        }
    }

    if (!isDirty) return;

    glBindFramebuffer(GL_FRAMEBUFFER, dirShadowFBO);
    glViewport(0, 0, m_CurrentShadowResolution, m_CurrentShadowResolution);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    bool reverseCull = ctx.directionalLight->reverseCullFace;

    auto ApplyCull = [&](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            if (reverseCull)
                glCullFace(mode == CullMode::Back ? GL_FRONT : GL_BACK);
            else
                glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
        }
    };

    auto BindMaterial = [](Shader& shader, MaterialComponent* material) {
        shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        shader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        shader.setVec4("albedoColor", material->GetAlbedoColor());
        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());

        bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
        shader.setBool("useAlbedoMap", useAlb);
        if (useAlb) {
            shader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
        }
        else {
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }
        return useAlb;
    };

    auto BindCustomUniforms = [](Shader& shader, const MaterialInstance& inst, const ShaderAsset& shaderAsset) {
        const auto& overrides = inst.GetCustomUniformOverrides();
        const auto& cached = inst.GetCachedCustomUniforms();

        for (const auto& uDef : shaderAsset.uniforms)
        {
            if (uDef.engineTexture != EngineTextureType::None || uDef.type == ShaderUniformType::Texture2D)
                continue;

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
            default:
                break;
            }
        }
    };

    Shader& shadowShader = ShaderManager::Get("DirectionalShadow");
    Shader& skeletalShadowShader = ShaderManager::Get("DirectionalShadow_Skeletal");

    for (int cascade = 0; cascade < NUM_SHADOW_CASCADES; ++cascade)
    {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_DirectionalShadowMap, 0, cascade);
        glClear(GL_DEPTH_BUFFER_BIT);

        const glm::mat4& cascadeMatrix = ctx.directionalLight->cascadeMatrices[cascade];

        if (!commands.empty())
        {
            Shader* lastBound = nullptr;

            for (auto& command : commands) {
                if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;
                if (!command.mesh->IsCastShadow()) continue;
                if (command.mesh->worldAABB.IsValid() && !ctx.camera->Intersects(cascadeMatrix, command.mesh->worldAABB)) continue;

                auto& material = command.material;
                if (material->GetTransparencyMode() == TransparencyMode::Alpha) continue;

                bool hasCustom = material->HasCustomShader();
                Ref<ShaderAsset> customAsset = hasCustom ? material->GetCustomShaderAsset() : nullptr;
                if (hasCustom && (!customAsset || !customAsset->compileSuccess || !customAsset->directionalShadowShader))
                    hasCustom = false;

                Shader* currentShader = hasCustom ? customAsset->directionalShadowShader.get() : &shadowShader;
                if (currentShader != lastBound)
                {
                    currentShader->use();
                    currentShader->setMat4("lightSpaceMatrix", cascadeMatrix);
                    if (!hasCustom)
                        currentShader->setInt("billboardMode", 0);
                    lastBound = currentShader;
                }

                if (hasCustom)
                {
                    BindCustomUniforms(*currentShader, material->GetInstance(), *customAsset);
                }

                CullMode effectiveCull = (hasCustom && customAsset->renderModes.hasCullMode)
                    ? customAsset->renderModes.cullMode
                    : material->GetCullMode();
                ApplyCull(effectiveCull);

                currentShader->setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
                bool useAlb = BindMaterial(*currentShader, material);

                currentShader->setMat4("model", command.transform->GetWorldMatrix());
                glm::mat3 normalMatrix = command.transform->GetNormalMatrix();
                currentShader->setMat3("normalMatrix", normalMatrix);
                currentShader->setFloat("u_VisibilityFade", command.visibilityFade);
                currentShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                currentShader->setVec2("UVScale", material->GetUVScale());
                currentShader->setVec2("UVOffset", material->GetUVOffset());

                command.mesh->Bind();
                glDrawElements(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (const void*)(uintptr_t)(command.indexOffset * sizeof(unsigned int)));

                if (useAlb) glBindSampler(0, 0);
            }
        }

        if (!ctx.directionalLight->staticShadow && !skeletalCommands.empty())
        {
            Shader* lastBound = nullptr;

            for (auto& command : skeletalCommands) {
                if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;
                if (!command.mesh->IsCastShadow()) continue;

                auto& material = command.material;
                if (material->GetTransparencyMode() == TransparencyMode::Alpha) continue;

                bool hasCustom = material->HasCustomShader();
                Ref<ShaderAsset> customAsset = hasCustom ? material->GetCustomShaderAsset() : nullptr;
                if (hasCustom && (!customAsset || !customAsset->compileSuccess || !customAsset->skeletalDirectionalShadowShader))
                    hasCustom = false;

                Shader* currentShader = hasCustom ? customAsset->skeletalDirectionalShadowShader.get() : &skeletalShadowShader;
                if (currentShader != lastBound)
                {
                    currentShader->use();
                    currentShader->setMat4("lightSpaceMatrix", cascadeMatrix);
                    lastBound = currentShader;
                }

                if (hasCustom)
                {
                    BindCustomUniforms(*currentShader, material->GetInstance(), *customAsset);
                }

                CullMode effectiveCull = (hasCustom && customAsset->renderModes.hasCullMode)
                    ? customAsset->renderModes.cullMode
                    : material->GetCullMode();
                ApplyCull(effectiveCull);

                bool useAlb = BindMaterial(*currentShader, material);

                currentShader->setMat4("model", command.transform->GetWorldMatrix());
                glm::mat3 normalMatrix = command.transform->GetNormalMatrix();
                currentShader->setMat3("normalMatrix", normalMatrix);
                currentShader->setFloat("u_VisibilityFade", command.visibilityFade);
                currentShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                currentShader->setVec2("UVScale", material->GetUVScale());
                currentShader->setVec2("UVOffset", material->GetUVOffset());

                GLint boneLoc = glGetUniformLocation(currentShader->ID, "u_BoneMatrices");
                if (boneLoc != -1 && command.skeleton && !command.skeleton->skinningPalette.empty())
                {
                    GLsizei uploadCount = static_cast<GLsizei>(std::min(command.skeleton->skinningPalette.size(), MAX_BONES));
                    glUniformMatrix4fv(
                        boneLoc,
                        uploadCount,
                        GL_FALSE,
                        glm::value_ptr(command.skeleton->skinningPalette[0])
                    );
                }

                command.mesh->Bind();
                glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                if (useAlb) glBindSampler(0, 0);
            }
        }
    }

    if (ctx.directionalLight->staticShadow)
    {
        m_CachedDirectionalShadow.valid = true;
        m_CachedDirectionalShadow.lightID = ctx.directionalLight->lightID;
        m_CachedDirectionalShadow.direction = ctx.directionalLight->direction;
        m_CachedDirectionalShadow.shadowBias = ctx.directionalLight->shadowBias;
        m_CachedDirectionalShadow.normalBias = ctx.directionalLight->normalBias;
        m_CachedDirectionalShadow.reverseCullFace = ctx.directionalLight->reverseCullFace;
        for (int i = 0; i < NUM_SHADOW_CASCADES; ++i)
            m_CachedDirectionalShadow.cascadeMatrices[i] = ctx.directionalLight->cascadeMatrices[i];
    }
    else
    {
        m_CachedDirectionalShadow = {};
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowPass::RenderPointShadowMap(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;

    bool anyNeedsRender = false;
    for (const auto& light : ctx.pointLights)
    {
        if (!light.castShadows || light.shadowIndex < 0 || light.shadowIndex >= MaxPointLights)
            continue;
        if (!light.staticShadow)
        {
            anyNeedsRender = true;
            break;
        }
        const auto& cached = m_CachedPointShadows[light.shadowIndex];
        if (!cached.valid || light.shadowDirty ||
            cached.lightID != light.lightID ||
            cached.position != light.position ||
            cached.radius != light.radius ||
            cached.shadowBias != light.shadowBias ||
            cached.reverseCullFace != light.reverseCullFace)
        {
            anyNeedsRender = true;
            break;
        }
    }
    if (!anyNeedsRender) return;

    glBindFramebuffer(GL_FRAMEBUFFER, pointFBO);
    glViewport(0, 0, POINT_SHADOW_WIDTH, POINT_SHADOW_HEIGHT);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    auto BindCustomUniforms = [](Shader& shader, const MaterialInstance& inst, const ShaderAsset& shaderAsset) {
        const auto& overrides = inst.GetCustomUniformOverrides();
        const auto& cached = inst.GetCachedCustomUniforms();

        for (const auto& uDef : shaderAsset.uniforms)
        {
            if (uDef.engineTexture != EngineTextureType::None || uDef.type == ShaderUniformType::Texture2D)
                continue;

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
            default:
                break;
            }
        }
    };

    Shader& pointShadowShader = ShaderManager::Get("PointShadow");
    Shader& skeletalPointShadowShader = ShaderManager::Get("PointShadow_Skeletal");

    auto BindMaterial = [](Shader& shader, MaterialComponent* material) {
        shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        shader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        shader.setVec4("albedoColor", material->GetAlbedoColor());
        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());

        bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
        shader.setBool("useAlbedoMap", useAlb);
        if (useAlb) {
            shader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
        }
        else {
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }
        return useAlb;
    };

    for (auto& light : ctx.pointLights)
    {
        if (!light.castShadows) continue;
        int shadowIdx = light.shadowIndex;
        if (shadowIdx < 0 || shadowIdx >= MaxPointLights) continue;

        bool isDirty = true;
        if (light.staticShadow)
        {
            const auto& cached = m_CachedPointShadows[shadowIdx];
            if (cached.valid &&
                !light.shadowDirty &&
                cached.lightID == light.lightID &&
                cached.position == light.position &&
                cached.radius == light.radius &&
                cached.shadowBias == light.shadowBias &&
                cached.reverseCullFace == light.reverseCullFace)
            {
                isDirty = false;
            }
        }

        if (!isDirty) continue;

        glm::vec3 lightPos = light.position;

        float near_plane = 0.05f;
        float far_plane = light.radius;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)POINT_SHADOW_WIDTH / (float)POINT_SHADOW_HEIGHT, near_plane, far_plane);
        std::array<glm::mat4, 6> shadowTransforms;
        shadowTransforms[0] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[1] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[2] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        shadowTransforms[3] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        shadowTransforms[4] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        shadowTransforms[5] = shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

        int baseLayer = light.shadowIndex * 6;

        for (unsigned int face = 0; face < 6; ++face)
        {
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_PointShadowMap, 0, baseLayer + face);
            glClear(GL_DEPTH_BUFFER_BIT);

            if (!commands.empty())
            {
                Shader* lastBound = nullptr;

                for (auto& command : commands)
                {
                    if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;
                    if (!command.mesh->IsCastShadow()) continue;
                    if (command.mesh->worldAABB.IsValid() && !ctx.camera->Intersects(shadowTransforms[face], command.mesh->worldAABB)) continue;

                    auto& material = command.material;
                    if (material->GetTransparencyMode() == TransparencyMode::Alpha) continue;

                    bool hasCustom = material->HasCustomShader();
                    Ref<ShaderAsset> customAsset = hasCustom ? material->GetCustomShaderAsset() : nullptr;
                    if (hasCustom && (!customAsset || !customAsset->compileSuccess || !customAsset->pointShadowShader))
                        hasCustom = false;

                    Shader* currentShader = hasCustom ? customAsset->pointShadowShader.get() : &pointShadowShader;
                    if (currentShader != lastBound)
                    {
                        currentShader->use();
                        currentShader->setFloat("far_plane", far_plane);
                        currentShader->setVec3("lightPos", lightPos);
                        currentShader->setInt("baseLayer", baseLayer);
                        currentShader->setInt("currentFace", face);
                        currentShader->setMat4("shadowMatrix", shadowTransforms[face]);
                        if (!hasCustom)
                            currentShader->setInt("billboardMode", 0);
                        lastBound = currentShader;
                    }

                    if (hasCustom)
                    {
                        BindCustomUniforms(*currentShader, material->GetInstance(), *customAsset);
                    }

                    CullMode effectiveCull = (hasCustom && customAsset->renderModes.hasCullMode)
                        ? customAsset->renderModes.cullMode
                        : material->GetCullMode();

                    if (effectiveCull == CullMode::Disabled) {
                        glDisable(GL_CULL_FACE);
                    }
                    else {
                        glEnable(GL_CULL_FACE);
                        if (light.reverseCullFace)
                            glCullFace(effectiveCull == CullMode::Back ? GL_FRONT : GL_BACK);
                        else
                            glCullFace(effectiveCull == CullMode::Back ? GL_BACK : GL_FRONT);
                    }

                    currentShader->setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
                    bool useAlb = BindMaterial(*currentShader, material);

                    currentShader->setMat4("model", command.transform->GetWorldMatrix());
                    glm::mat3 normalMatrix = command.transform->GetNormalMatrix();
                    currentShader->setMat3("normalMatrix", normalMatrix);
                currentShader->setFloat("u_VisibilityFade", command.visibilityFade);
                    currentShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                    currentShader->setVec2("UVScale", material->GetUVScale());
                    currentShader->setVec2("UVOffset", material->GetUVOffset());

                    command.mesh->Bind();
                    glDrawElements(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (const void*)(uintptr_t)(command.indexOffset * sizeof(unsigned int)));

                    if (useAlb) glBindSampler(0, 0);
                }
            }

            if (!light.staticShadow && !skeletalCommands.empty())
            {
                Shader* lastBound = nullptr;

                for (auto& command : skeletalCommands)
                {
                    if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;
                    if (!command.mesh->IsCastShadow()) continue;

                    auto& material = command.material;
                    if (material->GetTransparencyMode() == TransparencyMode::Alpha) continue;

                    bool hasCustom = material->HasCustomShader();
                    Ref<ShaderAsset> customAsset = hasCustom ? material->GetCustomShaderAsset() : nullptr;
                    if (hasCustom && (!customAsset || !customAsset->compileSuccess || !customAsset->skeletalPointShadowShader))
                        hasCustom = false;

                    Shader* currentShader = hasCustom ? customAsset->skeletalPointShadowShader.get() : &skeletalPointShadowShader;
                    if (currentShader != lastBound)
                    {
                        currentShader->use();
                        currentShader->setFloat("far_plane", far_plane);
                        currentShader->setVec3("lightPos", lightPos);
                        currentShader->setInt("baseLayer", baseLayer);
                        currentShader->setInt("currentFace", face);
                        currentShader->setMat4("shadowMatrix", shadowTransforms[face]);
                        lastBound = currentShader;
                    }

                    if (hasCustom)
                    {
                        BindCustomUniforms(*currentShader, material->GetInstance(), *customAsset);
                    }

                    CullMode effectiveCull = (hasCustom && customAsset->renderModes.hasCullMode)
                        ? customAsset->renderModes.cullMode
                        : material->GetCullMode();

                    if (effectiveCull == CullMode::Disabled) {
                        glDisable(GL_CULL_FACE);
                    }
                    else {
                        glEnable(GL_CULL_FACE);
                        if (light.reverseCullFace)
                            glCullFace(effectiveCull == CullMode::Back ? GL_FRONT : GL_BACK);
                        else
                            glCullFace(effectiveCull == CullMode::Back ? GL_BACK : GL_FRONT);
                    }

                    bool useAlb = BindMaterial(*currentShader, material);

                    currentShader->setMat4("model", command.transform->GetWorldMatrix());
                    glm::mat3 normalMatrix = command.transform->GetNormalMatrix();
                    currentShader->setMat3("normalMatrix", normalMatrix);
                currentShader->setFloat("u_VisibilityFade", command.visibilityFade);
                    currentShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                    currentShader->setVec2("UVScale", material->GetUVScale());
                    currentShader->setVec2("UVOffset", material->GetUVOffset());

                    GLint boneLoc = glGetUniformLocation(currentShader->ID, "u_BoneMatrices");
                    if (boneLoc != -1 && command.skeleton && !command.skeleton->skinningPalette.empty())
                    {
                        GLsizei uploadCount = static_cast<GLsizei>(std::min(command.skeleton->skinningPalette.size(), MAX_BONES));
                        glUniformMatrix4fv(
                            boneLoc,
                            uploadCount,
                            GL_FALSE,
                            glm::value_ptr(command.skeleton->skinningPalette[0])
                        );
                    }

                    command.mesh->Bind();
                    glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                    if (useAlb) glBindSampler(0, 0);
                }
            }
        }

        if (light.staticShadow)
        {
            m_CachedPointShadows[shadowIdx] = {
                true,
                light.lightID,
                light.position,
                light.radius,
                light.shadowBias,
                light.reverseCullFace
            };
        }
        else
        {
            m_CachedPointShadows[shadowIdx] = {};
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}

void ShadowPass::RenderSpotShadowMap(const RenderContext& ctx, RenderFrame& frame)
{
    if (ctx.spotLights.empty()) return;
    auto& commands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;

    bool anyNeedsRender = false;
    for (const auto& light : ctx.spotLights)
    {
        if (!light.castShadows || light.shadowIndex < 0 || light.shadowIndex >= MaxSpotLights)
            continue;
        if (!light.staticShadow)
        {
            anyNeedsRender = true;
            break;
        }
        const auto& cached = m_CachedSpotShadows[light.shadowIndex];
        if (!cached.valid || light.shadowDirty ||
            cached.lightID != light.lightID ||
            cached.position != light.position ||
            cached.direction != light.direction ||
            cached.radius != light.radius ||
            cached.innerCutoff != light.innerCutoff ||
            cached.outerCutoff != light.outerCutoff ||
            cached.shadowBias != light.shadowBias ||
            cached.reverseCullFace != light.reverseCullFace ||
            cached.lightSpaceMatrix != light.lightSpaceMatrix)
        {
            anyNeedsRender = true;
            break;
        }
    }
    if (!anyNeedsRender) return;

    glBindFramebuffer(GL_FRAMEBUFFER, spotFBO);
    glViewport(0, 0, SPOT_SHADOW_WIDTH, SPOT_SHADOW_HEIGHT);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 4.0f);

    auto BindMaterial = [](Shader& shader, MaterialComponent* material) {
        shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        shader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        shader.setVec4("albedoColor", material->GetAlbedoColor());
        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());

        bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
        shader.setBool("useAlbedoMap", useAlb);
        if (useAlb) {
            shader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
        }
        else {
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }
        return useAlb;
    };

    auto BindCustomUniforms = [](Shader& shader, const MaterialInstance& inst, const ShaderAsset& shaderAsset) {
        const auto& overrides = inst.GetCustomUniformOverrides();
        const auto& cached = inst.GetCachedCustomUniforms();

        for (const auto& uDef : shaderAsset.uniforms)
        {
            if (uDef.engineTexture != EngineTextureType::None || uDef.type == ShaderUniformType::Texture2D)
                continue;

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
            default:
                break;
            }
        }
    };

    Shader& shadowShader = ShaderManager::Get("DirectionalShadow");
    Shader& skeletalShadowShader = ShaderManager::Get("DirectionalShadow_Skeletal");

    for (const auto& light : ctx.spotLights)
    {
        if (!light.castShadows || light.shadowIndex < 0 || light.shadowIndex >= MaxSpotLights)
            continue;

        int shadowIdx = light.shadowIndex;
        bool isDirty = true;
        if (light.staticShadow)
        {
            const auto& cached = m_CachedSpotShadows[shadowIdx];
            if (cached.valid &&
                !light.shadowDirty &&
                cached.lightID == light.lightID &&
                cached.position == light.position &&
                cached.direction == light.direction &&
                cached.radius == light.radius &&
                cached.innerCutoff == light.innerCutoff &&
                cached.outerCutoff == light.outerCutoff &&
                cached.shadowBias == light.shadowBias &&
                cached.reverseCullFace == light.reverseCullFace &&
                cached.lightSpaceMatrix == light.lightSpaceMatrix)
            {
                isDirty = false;
            }
        }

        if (!isDirty) continue;

        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, o_SpotShadowMap, 0, light.shadowIndex);
        glClear(GL_DEPTH_BUFFER_BIT);

        bool reverseCull = light.reverseCullFace;
        auto ApplyCull = [&](CullMode mode) {
            if (mode == CullMode::Disabled) {
                glDisable(GL_CULL_FACE);
            }
            else {
                glEnable(GL_CULL_FACE);
                if (reverseCull)
                    glCullFace(mode == CullMode::Back ? GL_FRONT : GL_BACK);
                else
                    glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
            }
        };

        const glm::mat4& lightSpace = light.lightSpaceMatrix;

        if (!commands.empty())
        {
            Shader* lastBound = nullptr;

            for (auto& command : commands) {
                if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;
                if (!command.mesh->IsCastShadow()) continue;

                auto& material = command.material;
                if (material->GetTransparencyMode() == TransparencyMode::Alpha) continue;

                bool hasCustom = material->HasCustomShader();
                Ref<ShaderAsset> customAsset = hasCustom ? material->GetCustomShaderAsset() : nullptr;
                if (hasCustom && (!customAsset || !customAsset->compileSuccess || !customAsset->directionalShadowShader))
                    hasCustom = false;

                Shader* currentShader = hasCustom ? customAsset->directionalShadowShader.get() : &shadowShader;
                if (currentShader != lastBound)
                {
                    currentShader->use();
                    currentShader->setMat4("lightSpaceMatrix", lightSpace);
                    if (!hasCustom)
                        currentShader->setInt("billboardMode", 0);
                    lastBound = currentShader;
                }

                if (hasCustom)
                {
                    BindCustomUniforms(*currentShader, material->GetInstance(), *customAsset);
                }

                CullMode effectiveCull = (hasCustom && customAsset->renderModes.hasCullMode)
                    ? customAsset->renderModes.cullMode
                    : material->GetCullMode();
                ApplyCull(effectiveCull);

                currentShader->setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
                bool useAlb = BindMaterial(*currentShader, material);

                currentShader->setMat4("model", command.transform->GetWorldMatrix());
                glm::mat3 normalMatrix = command.transform->GetNormalMatrix();
                currentShader->setMat3("normalMatrix", normalMatrix);
                currentShader->setFloat("u_VisibilityFade", command.visibilityFade);
                currentShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                currentShader->setVec2("UVScale", material->GetUVScale());
                currentShader->setVec2("UVOffset", material->GetUVOffset());

                command.mesh->Bind();
                glDrawElements(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (const void*)(uintptr_t)(command.indexOffset * sizeof(unsigned int)));

                if (useAlb) glBindSampler(0, 0);
            }
        }

        if (!light.staticShadow && !skeletalCommands.empty())
        {
            Shader* lastBound = nullptr;

            for (auto& command : skeletalCommands) {
                if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;
                if (!command.mesh->IsCastShadow()) continue;

                auto& material = command.material;
                if (material->GetTransparencyMode() == TransparencyMode::Alpha) continue;

                bool hasCustom = material->HasCustomShader();
                Ref<ShaderAsset> customAsset = hasCustom ? material->GetCustomShaderAsset() : nullptr;
                if (hasCustom && (!customAsset || !customAsset->compileSuccess || !customAsset->skeletalDirectionalShadowShader))
                    hasCustom = false;

                Shader* currentShader = hasCustom ? customAsset->skeletalDirectionalShadowShader.get() : &skeletalShadowShader;
                if (currentShader != lastBound)
                {
                    currentShader->use();
                    currentShader->setMat4("lightSpaceMatrix", lightSpace);
                    lastBound = currentShader;
                }

                if (hasCustom)
                {
                    BindCustomUniforms(*currentShader, material->GetInstance(), *customAsset);
                }

                CullMode effectiveCull = (hasCustom && customAsset->renderModes.hasCullMode)
                    ? customAsset->renderModes.cullMode
                    : material->GetCullMode();
                ApplyCull(effectiveCull);

                bool useAlb = BindMaterial(*currentShader, material);

                currentShader->setMat4("model", command.transform->GetWorldMatrix());
                glm::mat3 normalMatrix = command.transform->GetNormalMatrix();
                currentShader->setMat3("normalMatrix", normalMatrix);
                currentShader->setFloat("u_VisibilityFade", command.visibilityFade);
                currentShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                currentShader->setVec2("UVScale", material->GetUVScale());
                currentShader->setVec2("UVOffset", material->GetUVOffset());

                GLint boneLoc = glGetUniformLocation(currentShader->ID, "u_BoneMatrices");
                if (boneLoc != -1 && command.skeleton && !command.skeleton->skinningPalette.empty())
                {
                    GLsizei uploadCount = static_cast<GLsizei>(std::min(command.skeleton->skinningPalette.size(), MAX_BONES));
                    glUniformMatrix4fv(
                        boneLoc,
                        uploadCount,
                        GL_FALSE,
                        glm::value_ptr(command.skeleton->skinningPalette[0])
                    );
                }

                command.mesh->Bind();
                glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                if (useAlb) glBindSampler(0, 0);
            }
        }

        if (light.staticShadow)
        {
            m_CachedSpotShadows[shadowIdx] = {
                true,
                light.lightID,
                light.position,
                light.direction,
                light.radius,
                light.innerCutoff,
                light.outerCutoff,
                light.shadowBias,
                light.reverseCullFace,
                light.lightSpaceMatrix
            };
        }
        else
        {
            m_CachedSpotShadows[shadowIdx] = {};
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);
}

void ShadowPass::RecreateDirectionalShadowMap(uint32_t resolution, RenderFrame& frame)
{
    m_CurrentShadowResolution = resolution;
    m_CachedDirectionalShadow = {};

    if (o_DirectionalShadowMap != 0)
        glDeleteTextures(1, &o_DirectionalShadowMap);
    if (dirShadowFBO != 0)
        glDeleteFramebuffers(1, &dirShadowFBO);

    InitDirectionalShadowMap();
    frame.registry.Register("DirectionalShadowMap", { RenderResourceType::Texture, o_DirectionalShadowMap });
}

void ShadowPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (ctx.scene != m_LastScene)
    {
        InvalidateCache();
        m_LastScene = ctx.scene;
    }

    if (ProjectManager::IsProjectLoaded())
    {
        uint32_t targetRes = ProjectManager::GetActiveProject()->GetSettings().shadowResolution;
        if (targetRes > 0 && targetRes != m_CurrentShadowResolution)
        {
            RecreateDirectionalShadowMap(targetRes, frame);
        }
    }

    RenderDirectionalShadowMap(ctx, frame);
    RenderPointShadowMap(ctx, frame);
    RenderSpotShadowMap(ctx, frame);
}

ShadowPass::~ShadowPass()
{
    glDeleteTextures(1, &o_DirectionalShadowMap);
    glDeleteFramebuffers(1, &dirShadowFBO);
    glDeleteTextures(1, &o_PointShadowMap);
    glDeleteFramebuffers(1, &pointFBO);
    glDeleteTextures(1, &o_SpotShadowMap);
    glDeleteFramebuffers(1, &spotFBO);
}
