#include "rvelapch.h"
#include "DebugPass.h"
#include "Rendering/DebugRenderer.h"
#include "Rendering/RenderContext.h"
#include "Rendering/Camera.h"
#include "Scene/Scene.h"

using namespace rv;

void DebugPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
	DebugRenderer::Get().Init();
}

void DebugPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    

    if (DebugRenderer::Get().GetSettings().drawBoundingBoxes)
        DrawAABBs(ctx);

    GLuint finalFramebuffer = frame.registry.Get("FinalFramebuffer")->id;
    GLuint depthTexture = frame.registry.Get("DepthTexture")->id;

    glBindFramebuffer(GL_FRAMEBUFFER, finalFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthTexture, 0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); 

    glm::mat4 mvp = ctx.camera->GetProjectionMatrix() * ctx.camera->GetViewMatrix();
    DebugRenderer::Get().EndFrame(mvp);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, 0, 0);

    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DebugPass::DrawAABBs(const RenderContext& ctx)
{
    auto& reg = ctx.scene->GetRegistry();
       
    auto view = reg.view<MeshRendererComponent>();
    for (auto e : view)
    {
        auto& comp = reg.get<MeshRendererComponent>(e);
        DebugRenderer::Get().DrawBox(comp.worldAABB.min, comp.worldAABB.max, { 0.0,1.0,1.0,1.0 });
    }
   

}
