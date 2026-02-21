#include "rvelapch.h"
#include "SkyboxPass.h"
#include "Rendering/RenderContext.h"
#include "Scene/ICamera.h"

namespace rv {

void SkyboxPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    Path path = VRT_PATH("Assets\\Textures\\skybox\\environment.hdr");
    m_Skybox.InitHDR(path, ShaderManager::Get("ToCubemap"));

    frame.registry.Register("SkyboxTexture", { RenderResourceType::Texture,m_Skybox.GetTextureID() });
}

SkyboxPass::~SkyboxPass()
{
}
void SkyboxPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto screenFBO = frame.registry.Get("ScreenBuffer")->id;
	m_Skybox.Render(ShaderManager::Get("Skybox"), ctx.camera->GetProjectionMatrix(), ctx.camera->GetViewMatrix(), screenFBO);
    frame.registry.Register("SkyboxTexture", { RenderResourceType::Texture,m_Skybox.GetTextureID() });

}

}