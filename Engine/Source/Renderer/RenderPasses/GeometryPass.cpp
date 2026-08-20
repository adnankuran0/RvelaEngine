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
    auto& commands = frame.opaqueCommands;
    auto& resourceRegistry = frame.registry;

    Shader& geometryShader = ShaderManager::Get("Geometry");

    m_GBuffer.BindViewport();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    geometryShader.use();

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
        }
        };

    for (auto& command : commands) {
        if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

        auto& material = command.material;
        ApplyCullMode(material->GetCullMode());

        glm::mat4 model = command.transform->GetWorldMatrix();

        geometryShader.setMat4("model", model);
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        geometryShader.setMat3("normalMatrix", normalMatrix);
        geometryShader.setVec2("UVScale", material->GetUVScale());
        geometryShader.setVec2("UVOffset", material->GetUVOffset());

        geometryShader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
        geometryShader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
        geometryShader.setFloat("alphaCutoff", material->GetAlphaCutoff());
        geometryShader.setVec4("albedoColor", material->GetAlbedoColor());

        // Alpha scissor
        bool useAlb = material->IsUsingAlbedoMap() && material->GetAlbedoTexture();
        geometryShader.setBool("useAlbedoMap", useAlb);
        if (useAlb)
        {
            geometryShader.setInt("albedoMap", 0);
            TextureCache::Get().GetOrCreate(material->GetAlbedoTexture()).Bind(0);
            material->GetSampler().Bind(0);
        }
        else
        {
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }

        // Roughness (Slot 1)
        if (material->IsUsingRoughnessMap() && material->GetRoughnessTexture())
        {
            geometryShader.setBool("useRoughnessMap", true);
            geometryShader.setInt("roughnessMap", 1);
            TextureCache::Get().GetOrCreate(material->GetRoughnessTexture()).Bind(1);
            material->GetSampler().Bind(1);
        }
        else
        {
            geometryShader.setBool("useRoughnessMap", false);
            geometryShader.setFloat("roughness", material->GetRoughness());
            glBindSampler(1, 0);
            glBindTextureUnit(1, 0);
        }

        // Metallic (Slot 2)
        if (material->IsUsingMetallicMap() && material->GetMetallicTexture())
        {
            geometryShader.setBool("useMetallicMap", true);
            geometryShader.setInt("metallicMap", 2);
            TextureCache::Get().GetOrCreate(material->GetMetallicTexture()).Bind(2);
            material->GetSampler().Bind(2);
        }
        else
        {
            geometryShader.setBool("useMetallicMap", false);
            geometryShader.setFloat("metallic", material->GetMetallic());
            glBindSampler(2, 0);
            glBindTextureUnit(2, 0);
        }

        command.mesh->VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

        glBindSampler(0, 0);
        glBindSampler(1, 0);
        glBindSampler(2, 0);
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

}