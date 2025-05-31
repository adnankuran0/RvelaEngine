#include "rvelapch.h"
#include "SkyboxPass.h"
void SkyboxPass::Execute()
{
	Renderer::GetSkybox().Render(Renderer::GetSkyboxShader(), ctx.camera->projection, ctx.camera->GetViewMatrix(),screenFBO);
}