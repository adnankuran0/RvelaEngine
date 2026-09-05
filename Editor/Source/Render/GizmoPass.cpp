#include "rvelapch.h"
#include "GizmoPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/Camera.h"
#include "Renderer/DebugRenderer.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"

using namespace rv;



void GizmoPass::Init(const RenderContext& ctx, RenderFrame& frame) 
{
    m_GizmoProj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1.0f);
}

void GizmoPass::Execute(const RenderContext& ctx, RenderFrame& frame) 
{

    if (ctx.scene->GetState() != SceneState::EDIT)
        return;

    auto* finalFboRes = frame.registry.Get("FinalFramebuffer");
    auto* depthTexRes = frame.registry.Get("DepthTexture");
    if (!finalFboRes || !depthTexRes) return;

    glBindFramebuffer(GL_FRAMEBUFFER, finalFboRes->id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexRes->id, 0);
    glViewport(0, 0, ctx.viewportWidth, ctx.viewportHeight);

    DebugRenderer::Get().BeginFrame();

    auto& registry = ctx.scene->GetRegistry();
    for (auto e : registry.view<TransformComponent, CameraComponent>())
    {
        auto& tc = registry.get<TransformComponent>(e);

        glm::mat4 targetCameraView = glm::inverse(tc.GetWorldMatrix());

        glm::mat4 invTargetViewProj = glm::inverse(m_GizmoProj * targetCameraView);

        DebugRenderer::Get().DrawFrustum(invTargetViewProj, glm::vec4(1.0f, 0.75f, 0.1f, 1.0f));
    }

    glm::mat4 editorViewProj = ctx.camera->GetProjectionMatrix() * ctx.camera->GetViewMatrix();
    DebugRenderer::Get().EndFrame(editorViewProj);

  
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}