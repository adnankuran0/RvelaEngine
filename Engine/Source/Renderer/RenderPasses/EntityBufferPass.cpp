#include "rvelapch.h"
#include "EntityBufferPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Camera.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/ShaderManager.h"
#include "Renderer/TextureCache.h"

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

void EntityBufferPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& opaqueCommands = frame.opaqueCommands;
    auto& transparentCommands = frame.transparentCommands;

    m_Framebuffer.BindViewport();

    GLuint clearValue = 0;
    glClearBufferuiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    Shader& shader = ShaderManager::Get("EntityBuffer");
    shader.use();

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
        }
        };

    auto RenderCommandList = [&](const auto& commands)
        {
            for (const auto& command : commands)
            {
                if (!ctx.camera->Intersects(command.mesh->worldAABB)) continue;

                auto& material = command.material;
                ApplyCullMode(material->GetCullMode());

                glm::mat4 model = command.transform->GetWorldMatrix();
                shader.setMat4("model", model);
                shader.setUInt("u_EntityID", static_cast<uint32_t>(command.entityID));

                shader.setInt("transparencyMode", static_cast<int>(material->GetTransparencyMode()));
                shader.setInt("billboardMode", static_cast<int>(material->GetBillboardMode()));
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

                command.mesh->VAO.Bind();
                glDrawElements(GL_TRIANGLES, command.mesh->indexCount, GL_UNSIGNED_INT, 0);

                if (useAlb) glBindSampler(0, 0);
            }
        };

    RenderCommandList(opaqueCommands);
    RenderCommandList(transparentCommands);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    Framebuffer::BindDefault();
}