#include "rvelapch.h"
#include "SelectedEntityMaskPass.h"
#include "Rendering/RenderContext.h"
#include "Rendering/Renderer.h"
#include "Rendering/RenderFrame.h"
#include "Scene/Camera.h"
#include "Scene/Scene.h"

namespace rv {

void SelectedEntityMaskPass::Init(const RenderContext& ctx, RenderFrame& frame) {
    glGenFramebuffers(1, &m_Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    glGenTextures(1, &o_MaskTexture);
    glBindTexture(GL_TEXTURE_2D, o_MaskTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ctx.viewportWidth, ctx.viewportHeight,
        0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, o_MaskTexture, 0);

    GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("SelectedEntityMaskPass FBO not complete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("SelectedEntityMask", { RenderResourceType::Texture, o_MaskTexture });
    frame.registry.Register("SelectedEntityMaskFBO", { RenderResourceType::Framebuffer, m_Framebuffer });
}


void SelectedEntityMaskPass::Execute(const RenderContext& ctx, RenderFrame& frame) {
    if (m_SelectedEntity == entt::null || ctx.scene->GetState() != SceneState::EDIT)
        return;

    auto& commands = frame.commands;

    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    GLfloat clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 0, clearColor);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    Shader& shader = ShaderManager::Get("Mask");
    shader.use();

    glm::mat4 projection = ctx.camera->GetProjectionMatrix();
    glm::mat4 view = ctx.camera->GetViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    shader.setVec4("u_Color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    for (auto& command : commands) {
        if (command.entityID != m_SelectedEntity)
            continue;

        glm::mat4 model = command.transform.GetWorldMatrix();
        shader.setMat4("model", model);

        command.mesh.VAO.Bind();
        glDrawElements(GL_TRIANGLES, command.mesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("SelectedEntityMask", { RenderResourceType::Texture, o_MaskTexture });
    frame.registry.Register("SelectedEntityMaskFBO", { RenderResourceType::Framebuffer, m_Framebuffer });

}

}