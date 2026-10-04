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
            float aspect = (camera->height > 0) ? (float(camera->width) / float(camera->height)) : 1.0f;
            float fov = (camera->FOV > 0.0f) ? camera->FOV : 60.0f;
            float nearP = (camera->NearClip > 0.001f) ? camera->NearClip : 0.1f;
            proj = glm::perspective(glm::radians(fov), aspect, nearP, camera->FarClip);
        }
        else
        {
            proj = camera->GetProjectionMatrix();
        }
       
        skybox.Render(proj, ctx.camera->GetViewMatrix(), screenFBO);
    }
    
    frame.registry.Register("SkyboxTexture", { RenderResourceType::Texture, skybox.GetEnvironmentMap() });
}

