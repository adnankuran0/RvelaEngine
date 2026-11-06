#include "rvelapch.h"
#include "GeometryPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"
#include "Asset/AssetManager.h"
#include "Core/Time.h"
#include "Scene/Environment.h"
#include <glm/gtc/type_ptr.hpp>

using namespace rv;

void GeometryPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = ctx.viewportWidth;
    desc.height = ctx.viewportHeight;
    desc.colorAttachments = {
        { FramebufferTextureFormat::RGB16F, FramebufferFilterMode::Nearest }, // Normal
        { FramebufferTextureFormat::R8,     FramebufferFilterMode::Nearest }, // Roughness
        { FramebufferTextureFormat::R8,     FramebufferFilterMode::Nearest }, // Metallic
    };
    desc.hasDepth = true;
    desc.depthAttachment = { FramebufferTextureFormat::Depth32F };

    m_GBuffer = Framebuffer(desc);

    frame.registry.Register("DepthTexture", { RenderResourceType::Texture, m_GBuffer.GetDepthAttachment() });
    frame.registry.Register("NormalTexture", { RenderResourceType::Texture, m_GBuffer.GetColorAttachment(0) });
    frame.registry.Register("RoughnessTexture", { RenderResourceType::Texture, m_GBuffer.GetColorAttachment(1) });
    frame.registry.Register("MetallicTexture", { RenderResourceType::Texture, m_GBuffer.GetColorAttachment(2) });
    frame.registry.Register("GBuffer", { RenderResourceType::Framebuffer, m_GBuffer.GetID() });
}

void GeometryPass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    if (m_GBuffer.GetWidth() == width && m_GBuffer.GetHeight() == height)
        return;

    m_GBuffer.Resize(width, height);
    frame.registry.Register("DepthTexture", { RenderResourceType::Texture, m_GBuffer.GetDepthAttachment() });
    frame.registry.Register("NormalTexture", { RenderResourceType::Texture, m_GBuffer.GetColorAttachment(0) });
    frame.registry.Register("RoughnessTexture", { RenderResourceType::Texture, m_GBuffer.GetColorAttachment(1) });
    frame.registry.Register("MetallicTexture", { RenderResourceType::Texture, m_GBuffer.GetColorAttachment(2) });
    frame.registry.Register("GBuffer", { RenderResourceType::Framebuffer, m_GBuffer.GetID() });
}

void GeometryPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& staticCommands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;
    auto& resourceRegistry = frame.registry;

    m_GBuffer.BindViewport();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool needsGBuffer = false;
    if (ctx.environment)
    {
        needsGBuffer = ctx.environment->SSAO || 
                       ctx.environment->SSR || 
                       ctx.environment->SSGI || 
                       ctx.environment->ContactShadows;
    }
    if (!needsGBuffer)
    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
        }
        };

    auto BindMaterialGBuffer = [](Shader& shader, MaterialComponent* material) {
        shader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());
        shader.setBool("useTriplanar", material->GetUseTriplanar());
        shader.setBool("useWorldTriplanar", material->GetUseWorldTriplanar());
        shader.setFloat("triplanarSharpness", material->GetTriplanarSharpness());
        shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        shader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        shader.setVec4("albedoColor", material->GetAlbedoColor());

        bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
        shader.setBool("useAlbedoMap", useAlb);
        if (useAlb)
        {
            shader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
        }
        else
        {
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }

        if (material->IsUsingRoughnessMap() && material->GetRoughnessTexture())
        {
            shader.setBool("useRoughnessMap", true);
            shader.setInt("roughnessMap", 1);
            TextureCache::Get().GetOrCreate(material->GetRoughnessTexture()).Bind(1);
            material->GetSampler().Bind(1);
        }
        else
        {
            shader.setBool("useRoughnessMap", false);
            shader.setFloat("roughness", material->GetRoughness());
            glBindSampler(1, 0);
            glBindTextureUnit(1, 0);
        }

        if (material->IsUsingMetallicMap() && material->GetMetallicTexture())
        {
            shader.setBool("useMetallicMap", true);
            shader.setInt("metallicMap", 2);
            TextureCache::Get().GetOrCreate(material->GetMetallicTexture()).Bind(2);
            material->GetSampler().Bind(2);
        }
        else
        {
            shader.setBool("useMetallicMap", false);
            shader.setFloat("metallic", material->GetMetallic());
            glBindSampler(2, 0);
            glBindTextureUnit(2, 0);
        }

        bool useNorm = material->IsUsingNormalMap() && material->GetNormalTexture();
        shader.setBool("useNormalMap", useNorm);
        shader.setFloat("normalScale", material->GetNormalScale());
        if (useNorm)
        {
            shader.setInt("normalMap", 3);
            TextureCache::Get().GetOrCreate(material->GetNormalTexture()).Bind(3);
            material->GetSampler().Bind(3);
        }
        else
        {
            glBindSampler(3, 0);
            glBindTextureUnit(3, 0);
        }

        bool useHeight = material->IsUsingHeightMap() && material->GetHeightTexture();
        shader.setBool("useHeightMap", useHeight);
        shader.setFloat("heightScale", material->GetHeightScale());
        shader.setBool("invertHeight", material->GetInvertHeight());
        if (useHeight)
        {
            shader.setInt("heightMap", 5);
            TextureCache::Get().GetOrCreate(material->GetHeightTexture()).Bind(5);
            material->GetSampler().Bind(5);
        }
        else
        {
            glBindSampler(5, 0);
            glBindTextureUnit(5, 0);
        }
        };

    auto CleanupSamplers = []() {
        glBindSampler(0, 0);
        glBindSampler(1, 0);
        glBindSampler(2, 0);
        glBindSampler(3, 0);
        glBindSampler(5, 0);
        glBindTextureUnit(0, 0);
        glBindTextureUnit(1, 0);
        glBindTextureUnit(2, 0);
        glBindTextureUnit(3, 0);
        glBindTextureUnit(5, 0);
        };

    auto BindCustomShaderUniforms = [](Shader& shader, const MaterialInstance& inst, const ShaderAsset& shaderAsset) {
        int nextTextureSlot = 18;
        const auto& overrides = inst.GetCustomUniformOverrides();
        const auto& cached = inst.GetCachedCustomUniforms();

        for (const auto& uDef : shaderAsset.uniforms)
        {
            if (uDef.engineTexture != EngineTextureType::None)
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
                }
                nextTextureSlot++;
                break;
            }
        }
    };

    if (!staticCommands.empty())
    {
        Shader& defaultGeometryShader = ShaderManager::Get("Geometry");
        Shader* activeShader = nullptr;

        for (auto& command : staticCommands) {
            if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;
            if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

            auto& material = command.material;
            auto customShaderAsset = material->GetInstance().GetCustomShaderAsset();
            bool hasCustomGBuffer = (customShaderAsset && customShaderAsset->IsValid() && customShaderAsset->gBufferShader);

            Shader* targetShader = hasCustomGBuffer ? customShaderAsset->gBufferShader.get() : &defaultGeometryShader;
            if (activeShader != targetShader)
            {
                targetShader->use();
                activeShader = targetShader;
            }

            if (hasCustomGBuffer && customShaderAsset->renderModes.hasCullMode)
                ApplyCullMode(customShaderAsset->renderModes.cullMode);
            else
                ApplyCullMode(material->GetCullMode());

            glm::mat4 model = command.transform->GetWorldMatrix();
            targetShader->setMat4("model", model);
            targetShader->setFloat("u_VisibilityFade", command.visibilityFade);

            if (!needsGBuffer)
            {
                command.mesh->Bind();
                glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);
                continue;
            }

            targetShader->setMat3("normalMatrix", command.transform->GetNormalMatrix());

            if (hasCustomGBuffer)
            {
                targetShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                targetShader->setVec2("UVScale", material->GetUVScale());
                targetShader->setVec2("UVOffset", material->GetUVOffset());
                BindCustomShaderUniforms(*targetShader, material->GetInstance(), *customShaderAsset);
            }
            else
            {
                BindMaterialGBuffer(*targetShader, material);
            }

            command.mesh->Bind();
            glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

            CleanupSamplers();
        }
    }

    if (!skeletalCommands.empty())
    {
        Shader& defaultSkeletalGeometryShader = ShaderManager::Get("Geometry_Skeletal");
        Shader* activeShader = nullptr;

        for (auto& command : skeletalCommands) {
            if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;
            if (command.mesh->worldAABB.IsValid() && !ctx.camera->Intersects(command.mesh->worldAABB)) continue;

            auto& material = command.material;
            auto customShaderAsset = material->GetInstance().GetCustomShaderAsset();
            bool hasCustomGBuffer = (customShaderAsset && customShaderAsset->IsValid() && customShaderAsset->skeletalGBufferShader);

            Shader* targetShader = hasCustomGBuffer ? customShaderAsset->skeletalGBufferShader.get() : &defaultSkeletalGeometryShader;
            if (activeShader != targetShader)
            {
                targetShader->use();
                activeShader = targetShader;
            }

            if (hasCustomGBuffer && customShaderAsset->renderModes.hasCullMode)
                ApplyCullMode(customShaderAsset->renderModes.cullMode);
            else
                ApplyCullMode(material->GetCullMode());

            glm::mat4 model = command.transform->GetWorldMatrix();
            targetShader->setMat4("model", model);
            targetShader->setFloat("u_VisibilityFade", command.visibilityFade);

            GLint boneLoc = glGetUniformLocation(targetShader->ID, "u_BoneMatrices");
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

            if (!needsGBuffer)
            {
                command.mesh->Bind();
                glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);
                continue;
            }

            targetShader->setMat3("normalMatrix", command.transform->GetNormalMatrix());

            if (hasCustomGBuffer)
            {
                targetShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                targetShader->setVec2("UVScale", material->GetUVScale());
                targetShader->setVec2("UVOffset", material->GetUVOffset());
                BindCustomShaderUniforms(*targetShader, material->GetInstance(), *customShaderAsset);
            }
            else
            {
                BindMaterialGBuffer(*targetShader, material);
            }

            command.mesh->Bind();
            glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

            CleanupSamplers();
        }
    }

    if (!needsGBuffer)
    {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}