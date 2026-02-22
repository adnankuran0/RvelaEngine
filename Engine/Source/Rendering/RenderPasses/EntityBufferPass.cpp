#include "rvelapch.h"
#include "EntityBufferPass.h"
#include "Rendering/RenderContext.h"
#include "Scene/Camera.h"
#include <Rendering/RenderFrame.h>

namespace rv {

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

    frame.registry.Register("EntityBuffer", { RenderResourceType::Framebuffer,m_Framebuffer });
    frame.registry.Register("EntityTexture", { RenderResourceType::Framebuffer,o_EntityTexture });
}


void EntityBufferPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto& commands = frame.commands;
    auto& resourceRegistry = frame.registry;

    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    GLuint clearValue = 0;
    glClearBufferuiv(GL_COLOR, 0, &clearValue);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    Shader& shader = ShaderManager::Get("EntityBuffer");
    shader.use();

    glm::mat4 projection = ctx.camera->GetProjectionMatrix();
    glm::mat4 view = ctx.camera->GetViewMatrix();

    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    for (auto& command : commands)
    {
        if (!ctx.camera->Intersects(command.mesh.worldAABB)) continue;

        glm::mat4 model = command.transform.GetWorldMatrix();
        shader.setMat4("model", model);

      
        shader.setUInt("u_EntityID", static_cast<uint32_t>(command.entityID));

        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);

    }


    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    resourceRegistry.Register("EntityBuffer", { RenderResourceType::Framebuffer,m_Framebuffer });
    frame.registry.Register("EntityTexture", { RenderResourceType::Framebuffer,o_EntityTexture });

}


}