#include "rvelapch.h"
#include "SkyboxPass.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Camera.h"
#include "Scene/Environment.h"
#include "Asset/AssetRegistry.h"

using namespace rv;

void SkyboxPass::Init(const RenderContext& ctx, RenderFrame& frame)
{
    auto& skybox = ctx.environment->GetSkybox();
    if (skybox.GetEnvironmentMap() == 0)
    {
        if (skybox.GetHDRUUID().IsValid())
        {
            skybox.InitHDR(skybox.GetHDRUUID());
        }
        else if (skybox.GetPath().IsValid())
        {
            skybox.InitHDR(skybox.GetPath());
        }
        else
        {
            Path defaultPath = VRT_PATH("Assets\\Textures\\skybox\\environment.hdr");
            if (std::filesystem::exists(defaultPath.GetAbsolute()))
            {
                skybox.InitHDR(defaultPath);
            }
        }
    }

    frame.registry.Register("SkyboxTexture", { RenderResourceType::Texture, skybox.GetEnvironmentMap() });
}

SkyboxPass::~SkyboxPass()
{
}

void SkyboxPass::Execute(const RenderContext& ctx, RenderFrame& frame)
{
    auto screenFBO = frame.registry.Get("ScreenBuffer")->id;
    auto& skybox = ctx.environment->GetSkybox();

    if (skybox.GetEnvironmentMap() != 0)
    {
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
    }
    
    frame.registry.Register("SkyboxTexture", { RenderResourceType::Texture, skybox.GetEnvironmentMap() });
}

