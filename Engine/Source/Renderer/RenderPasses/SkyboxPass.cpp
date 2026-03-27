#include "rvelapch.h"
#include "SkyboxPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Camera.h"
#include "Scene/Environment.h"

using namespace rv;

void SkyboxPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    Path path = VRT_PATH("Assets\\Textures\\skybox\\environment.hdr");
    auto& skybox = ctx.environment->GetSkybox();
    skybox.InitHDR(path);

    frame.registry.Register("SkyboxTexture", { RenderResourceType::Texture,skybox.GetEnvironmentMap() });
}

SkyboxPass::~SkyboxPass()
{
}
void SkyboxPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
  

    auto screenFBO = frame.registry.Get("ScreenBuffer")->id;
    auto& skybox = ctx.environment->GetSkybox();

 
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
   
    skybox.Render(ctx.camera->GetProjectionMatrix(), ctx.camera->GetViewMatrix(), screenFBO);
    
    frame.registry.Register("SkyboxTexture", { RenderResourceType::Texture,skybox.GetEnvironmentMap() });
   
}

