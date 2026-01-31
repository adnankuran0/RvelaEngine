#include "rvelapch.h"
#include "SkyboxPass.h"

void SkyboxPass::Init()
{
    Path path = VRT_PATH("Assets\\Textures\\skybox\\environment.hdr");
    m_Skybox.InitHDR(path, Renderer::GetEquirectangularToCubemapShader());
}

SkyboxPass::~SkyboxPass()
{
}
void SkyboxPass::Execute()
{

	m_Skybox.Render(Renderer::GetSkyboxShader(), ctx.camera->GetProjectionMatrix(), ctx.camera->GetViewMatrix(), i_ScreenFBO);
}