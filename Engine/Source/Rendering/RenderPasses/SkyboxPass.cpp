#include "rvelapch.h"
#include "SkyboxPass.h"
#include "Rendering/RenderContext.h"
#include "Rendering/Camera.h"

using namespace rv;

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
    glm::mat4 proj;

    auto& camera = ctx.camera;
    if (camera->ProjectionType == Camera::Projection::Orthographic)
    {
        float aspect = float(camera->width) / float(camera->height);
        proj = glm::perspective(glm::radians(camera->FOV), aspect, camera->NearClip, camera->FarClip);
    }
    else
    {
        proj = camera->GetProjectionMatrix();
    }
	m_Skybox.Render(ShaderManager::Get("Skybox"), ctx.camera->GetProjectionMatrix(), ctx.camera->GetViewMatrix(), screenFBO);
    frame.registry.Register("SkyboxTexture", { RenderResourceType::Texture,m_Skybox.GetTextureID() });

}

