#include "rvelapch.h"
#include "EntityBufferPass.h"
#include "../Renderer.h"
#include "Scene/ICamera.h"

namespace rv {

void EntityBufferPass::Init()
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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void EntityBufferPass::Execute()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    GLuint clearValue = 0;
    glClearBufferuiv(GL_COLOR, 0, &clearValue);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // don't rewrite depth

    Shader& shader = Renderer::GetEntityBufferShader();
    shader.use();

    glm::mat4 projection = ctx.camera->GetProjectionMatrix();
    glm::mat4 view = ctx.camera->GetViewMatrix();

    shader.setMat4("view", view);
    shader.setMat4("projection", projection);

    for (auto& command : commands)
    {
        if (!ctx.camera->Intersects(command.mesh.worldAABB))
            continue;

        glm::mat4 model = command.transform.GetWorldMatrix();
        shader.setMat4("model", model);

      
        shader.setUInt("u_EntityID", static_cast<uint32_t>(command.entityID));

        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


}