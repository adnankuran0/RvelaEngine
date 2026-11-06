#include "rvelapch.h"
#include "EntityBufferPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"
#include "Scene/Scene.h"
#include "Asset/AssetManager.h"
#include "Core/Time.h"

using namespace rv;

void EntityBufferPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    FramebufferDesc desc;
    desc.width = ctx.viewportWidth;
    desc.height = ctx.viewportHeight;
    desc.colorAttachments = {
        { FramebufferTextureFormat::R32UI, FramebufferFilterMode::Nearest }
    };
    desc.hasDepth = true;
    desc.depthAsRenderbuffer = true;
    desc.depthAttachment = { FramebufferTextureFormat::Depth24 };

    m_Framebuffer = Framebuffer(desc);

    frame.registry.Register("EntityBuffer", { RenderResourceType::Framebuffer, m_Framebuffer.GetID() });
    frame.registry.Register("EntityTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

void EntityBufferPass::OnResize(uint32_t width, uint32_t height, RenderFrame& frame)
{
    if (m_Framebuffer.GetWidth() == width && m_Framebuffer.GetHeight() == height)
        return;

    m_Framebuffer.Resize(width, height);
    frame.registry.Register("EntityBuffer", { RenderResourceType::Framebuffer, m_Framebuffer.GetID() });
    frame.registry.Register("EntityTexture", { RenderResourceType::Texture, m_Framebuffer.GetColorAttachment(0) });
}

void EntityBufferPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    if (ctx.profile == RenderProfile::RUNTIME || !ctx.scene || ctx.scene->GetState() != SceneState::EDIT)
        return;
    auto& opaqueCommands = frame.opaqueCommands;
    auto& transparentCommands = frame.transparentCommands;
    auto& skeletalOpaqueCommands = frame.skeletalOpaqueCommands;
    auto& skeletalTransparentCommands = frame.skeletalTransparentCommands;

    m_Framebuffer.BindViewport();

    GLuint clearValue = 0;
    glClearBufferuiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
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

    {
        Shader& defaultShader = ShaderManager::Get("EntityBuffer");
        Shader* activeShader = nullptr;

        auto RenderCommandList = [&](const auto& commands)
            {
                for (const auto& command : commands)
                {
                    if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

                    auto& material = command.material;
                    auto customShaderAsset = material->GetInstance().GetCustomShaderAsset();
                    bool hasCustom = (customShaderAsset && customShaderAsset->IsValid() && customShaderAsset->entityBufferShader);

                    Shader* targetShader = hasCustom ? customShaderAsset->entityBufferShader.get() : &defaultShader;
                    if (activeShader != targetShader)
                    {
                        targetShader->use();
                        activeShader = targetShader;
                    }

                    if (hasCustom && customShaderAsset->renderModes.hasCullMode)
                        ApplyCullMode(customShaderAsset->renderModes.cullMode);
                    else
                        ApplyCullMode(material->GetCullMode());

                    glm::mat4 model = command.transform->GetWorldMatrix();
                    targetShader->setMat4("model", model);
                    targetShader->setUInt("u_EntityID", static_cast<uint32_t>(command.entityID));

                    bool useAlb = false;
                    if (hasCustom)
                    {
                        targetShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                        targetShader->setVec2("UVScale", material->GetUVScale());
                        targetShader->setVec2("UVOffset", material->GetUVOffset());
                        BindCustomUniforms(*targetShader, material->GetInstance(), *customShaderAsset);
                    }
                    else
                    {
                        targetShader->setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
                        useAlb = BindMaterial(*targetShader, material);
                    }

                    command.mesh->Bind();
                    glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                    if (useAlb) glBindSampler(0, 0);
                }
            };

        RenderCommandList(opaqueCommands);
        RenderCommandList(transparentCommands);
    }

    if (!skeletalOpaqueCommands.empty() || !skeletalTransparentCommands.empty())
    {
        Shader& defaultSkeletalShader = ShaderManager::Get("EntityBuffer_Skeletal");
        Shader* activeShader = nullptr;

        auto RenderSkeletalCommandList = [&](const auto& commands)
            {
                for (const auto& command : commands)
                {
                    if (!command.mesh || command.mesh->indexCount == 0 || !command.mesh->gpuMesh) continue;

                    auto& material = command.material;
                    auto customShaderAsset = material->GetInstance().GetCustomShaderAsset();
                    bool hasCustom = (customShaderAsset && customShaderAsset->IsValid() && customShaderAsset->skeletalEntityBufferShader);

                    Shader* targetShader = hasCustom ? customShaderAsset->skeletalEntityBufferShader.get() : &defaultSkeletalShader;
                    if (activeShader != targetShader)
                    {
                        targetShader->use();
                        activeShader = targetShader;
                    }

                    if (hasCustom && customShaderAsset->renderModes.hasCullMode)
                        ApplyCullMode(customShaderAsset->renderModes.cullMode);
                    else
                        ApplyCullMode(material->GetCullMode());

                    glm::mat4 model = command.transform->GetWorldMatrix();
                    targetShader->setMat4("model", model);
                    targetShader->setUInt("u_EntityID", static_cast<uint32_t>(command.entityID));

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

                    bool useAlb = false;
                    if (hasCustom)
                    {
                        targetShader->setFloat("u_Time", static_cast<float>(Time::GetCurrentTime()));
                        targetShader->setVec2("UVScale", material->GetUVScale());
                        targetShader->setVec2("UVOffset", material->GetUVOffset());
                        BindCustomUniforms(*targetShader, material->GetInstance(), *customShaderAsset);
                    }
                    else
                    {
                        useAlb = BindMaterial(*targetShader, material);
                    }

                    command.mesh->Bind();
                    glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                    if (useAlb) glBindSampler(0, 0);
                }
            };

        RenderSkeletalCommandList(skeletalOpaqueCommands);
        RenderSkeletalCommandList(skeletalTransparentCommands);
    }

    if (ctx.scene)
    {
        ctx.scene->GetUISystem().RenderEntityIDs(ctx.scene, static_cast<float>(ctx.viewportWidth), static_cast<float>(ctx.viewportHeight));
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBindVertexArray(0);
    glUseProgram(0);
    Framebuffer::BindDefault();
}