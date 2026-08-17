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
    glGenFramebuffers(1, &m_Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    glGenTextures(1, &o_EntityTexture);
    glBindTexture(GL_TEXTURE_2D, o_EntityTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R32UI,
        ctx.viewportWidth,
        ctx.viewportHeight,
        0,
        GL_RED_INTEGER,
        GL_UNSIGNED_INT,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        o_EntityTexture,
        0
    );

    glGenRenderbuffers(1, &m_Renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_Renderbuffer);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH_COMPONENT24,
        ctx.viewportWidth,
        ctx.viewportHeight
    );

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        m_Renderbuffer
    );

    GLenum buffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("EntityBuffer FBO not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("EntityBuffer", { RenderResourceType::Framebuffer, m_Framebuffer });
    frame.registry.Register("EntityTexture", { RenderResourceType::Texture, o_EntityTexture });
}

void EntityBufferPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& opaqueCommands = frame.opaqueCommands;
    auto& transparentCommands = frame.transparentCommands;
    auto& resourceRegistry = frame.registry;

    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    GLuint clearValue = 0;
    glClearBufferuiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    Shader& shader = ShaderManager::Get("EntityBuffer");
    shader.use();

    glm::mat4 projection = ctx.camera->GetProjectionMatrix();
    glm::mat4 view = ctx.camera->GetViewMatrix();

    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setVec3("camPos", ctx.camera->Position);

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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    resourceRegistry.Register("EntityBuffer", { RenderResourceType::Framebuffer, m_Framebuffer });
    frame.registry.Register("EntityTexture", { RenderResourceType::Texture, o_EntityTexture });
}