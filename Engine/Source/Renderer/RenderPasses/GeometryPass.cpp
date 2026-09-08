#include "rvelapch.h"
#include "GeometryPass.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"

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
}

void GeometryPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& staticCommands = frame.opaqueCommands;
    auto& skeletalCommands = frame.skeletalOpaqueCommands;
    auto& resourceRegistry = frame.registry;

    m_GBuffer.BindViewport();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
        shader.setVec2("UVScale", material->GetUVScale());
        shader.setVec2("UVOffset", material->GetUVOffset());
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
        };

    auto CleanupSamplers = []() {
        glBindSampler(0, 0);
        glBindSampler(1, 0);
        glBindSampler(2, 0);
        };

    if (!staticCommands.empty())
    {
        Shader& geometryShader = ShaderManager::Get("Geometry");
        geometryShader.use();

        for (auto& command : staticCommands) {
            if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

            auto& material = command.material;
            ApplyCullMode(material->GetCullMode());

            glm::mat4 model = command.transform->GetWorldMatrix();
            geometryShader.setMat4("model", model);
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
            geometryShader.setMat3("normalMatrix", normalMatrix);

            BindMaterialGBuffer(geometryShader, material);

            command.mesh->VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

            CleanupSamplers();
        }
    }

    if (!skeletalCommands.empty())
    {
        Shader& skeletalGeometryShader = ShaderManager::Get("Geometry_Skeletal");
        skeletalGeometryShader.use();

        GLint boneLoc = glGetUniformLocation(skeletalGeometryShader.ID, "u_BoneMatrices");

        for (auto& command : skeletalCommands) {
            if (!command.mesh || command.mesh->indexCount == 0) continue;
            // if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

            auto& material = command.material;
            ApplyCullMode(material->GetCullMode());

            glm::mat4 model = command.transform->GetWorldMatrix();
            skeletalGeometryShader.setMat4("model", model);
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
            skeletalGeometryShader.setMat3("normalMatrix", normalMatrix);

            if (boneLoc != -1 && command.skeleton && !command.skeleton->skinningPalette.empty())
            {
                glUniformMatrix4fv(
                    boneLoc,
                    static_cast<GLsizei>(command.skeleton->skinningPalette.size()),
                    GL_FALSE,
                    glm::value_ptr(command.skeleton->skinningPalette[0])
                );
            }

            BindMaterialGBuffer(skeletalGeometryShader, material);

            command.mesh->VAO.Bind();
            glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

            CleanupSamplers();
        }
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}