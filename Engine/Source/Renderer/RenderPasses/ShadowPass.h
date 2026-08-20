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
    void InitPointShadowMap();
    void RenderDirectionalShadowMap(const RenderContext& ctx, RenderFrame& frame);
    void RenderPointShadowMap(const RenderContext& ctx, RenderFrame& frame);

    Framebuffer m_DirectionalShadowFramebuffer;

    GLuint pointFBO = 0;
    GLuint o_PointShadowMap = 0;

    const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
    const unsigned int POINT_SHADOW_WIDTH = 512, POINT_SHADOW_HEIGHT = 512;
};

}