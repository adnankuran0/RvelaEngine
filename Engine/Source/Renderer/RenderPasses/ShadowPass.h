#pragma once
#include "../RenderPass.h"
#include "Renderer/Camera.h"
#include "Renderer/Framebuffer.h"

namespace rv {

class ShadowPass : public RenderPass
{
public:
    ~ShadowPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
private:
    void InitDirectionalShadowMap();
    void InitPointShadowMap();
    void InitSpotShadowMap();
    void RenderDirectionalShadowMap(const RenderContext& ctx, RenderFrame& frame);
    void RenderPointShadowMap(const RenderContext& ctx, RenderFrame& frame);
    void RenderSpotShadowMap(const RenderContext& ctx, RenderFrame& frame);

    GLuint dirShadowFBO = 0;
    GLuint o_DirectionalShadowMap = 0;

    GLuint pointFBO = 0;
    GLuint o_PointShadowMap = 0;

    GLuint spotFBO = 0;
    GLuint o_SpotShadowMap = 0;

    uint32_t m_CurrentShadowResolution = 2048;
    const unsigned int POINT_SHADOW_WIDTH = 512, POINT_SHADOW_HEIGHT = 512;
    const unsigned int SPOT_SHADOW_WIDTH = 1024, SPOT_SHADOW_HEIGHT = 1024;
    void RecreateDirectionalShadowMap(uint32_t resolution, RenderFrame& frame);
};

}