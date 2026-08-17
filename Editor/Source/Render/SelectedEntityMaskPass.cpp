#include "rvelapch.h"
#include "SelectedEntityMaskPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/Camera.h"
#include "Scene/Scene.h"
#include "Renderer/TextureCache.h"

using namespace rv;

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

    auto& opaqueCommands = frame.opaqueCommands;
    auto& transparentCommands = frame.transparentCommands;

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
    shader.setVec3("camPos", ctx.camera->Position);
    shader.setVec4("u_Color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto ApplyCullMode = [](CullMode mode) {
        if (mode == CullMode::Disabled) {
            glDisable(GL_CULL_FACE);
        }
        else {
            glEnable(GL_CULL_FACE);
            glCullFace(mode == CullMode::Back ? GL_BACK : GL_FRONT);
        }
        };

    auto RenderMaskList = [&](const auto& commands) {
        for (auto& command : commands) {
            if (command.entityID != m_SelectedEntity)
                continue;

            auto& material = command.material;
            ApplyCullMode(material->GetCullMode());

            glm::mat4 model = command.transform->GetWorldMatrix();
            shader.setMat4("model", model);

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

    RenderMaskList(opaqueCommands);
    RenderMaskList(transparentCommands);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frame.registry.Register("SelectedEntityMask", { RenderResourceType::Texture, o_MaskTexture });
    frame.registry.Register("SelectedEntityMaskFBO", { RenderResourceType::Framebuffer, m_Framebuffer });
}