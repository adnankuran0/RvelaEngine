#include "rvelapch.h"
#include "DebugPass.h"
#include "Rendering/DebugRenderer/DebugRenderer.h"
#include "Rendering/RenderContext.h"
#include "Rendering/Camera.h"

using namespace rv;

void DebugPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
	DebugRenderer::Get().Init();
}

void DebugPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
	DebugRenderer::Get().DrawLine({0.0,0.0,0.0},{0.0,2.0,0.0},{1.0,0.0,0.0,1.0});

	GLuint finalFramebuffer = frame.registry.Get("FinalFramebuffer")->id;
	glBindFramebuffer(GL_FRAMEBUFFER, finalFramebuffer);

	glm::mat4 mvp = ctx.camera->GetProjectionMatrix() * ctx.camera->GetViewMatrix();
	DebugRenderer::Get().EndFrame(mvp);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


}
