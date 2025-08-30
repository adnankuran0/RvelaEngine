#include "rvelapch.h"
#include "OutlinePass.h"

void OutlinePass::Execute()
{
    if (!ctx.scene) return;
    entt::entity selectedEntity = ctx.scene->GetSelectedEntity();
    if (selectedEntity == entt::null || !ctx.scene->GetRegistry().valid(selectedEntity)) return;

    auto& registry = ctx.scene->GetRegistry();
    if (!registry.all_of<TransformComponent, MeshRendererComponent>(selectedEntity)) return;

    glBindFramebuffer(GL_FRAMEBUFFER, i_ScreenFBO);

    auto& transform = registry.get<TransformComponent>(selectedEntity);
    auto& meshRenderer = registry.get<MeshRendererComponent>(selectedEntity);

    glm::mat4 model = glm::scale(transform.GetLocalMatrix(), glm::vec3(1.05f));

    Shader& outlineShader = Renderer::GetOutlineShader();
    outlineShader.use();
    outlineShader.setMat4("u_Model", model);
    outlineShader.setMat4("u_View", ctx.camera->GetViewMatrix());
    outlineShader.setMat4("u_Projection", ctx.camera->projection);

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF); 
    glStencilMask(0x00);                 
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);                

    meshRenderer.VAO.Bind();
    glDrawElements(GL_TRIANGLES, meshRenderer.indexCount, GL_UNSIGNED_INT, 0);

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);

    
}