#include "rvelapch.h"
#include "OutlinePass.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Rendering/RenderContext.h"

namespace rv {

void OutlinePass::Init(const RenderContext& ctx, RenderFrame& frame)
{

}

void OutlinePass::Execute(const RenderContext& ctx, RenderFrame& frame) {
    if (m_SelectedEntity == entt::null || ctx.scene->GetState() != SceneState::EDIT)
        return;

    auto* maskRes = frame.registry.Get("SelectedEntityMask");
    auto* finalFboRes = frame.registry.Get("FinalFramebuffer");

    if (!maskRes || !finalFboRes)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, finalFboRes->id);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader& shader = Renderer::GetOutlineShader();
    shader.use();
    glBindTextureUnit(0, maskRes->id);

    Renderer::DrawFullScreenQuad();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}