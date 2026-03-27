#pragma once
#include "../RenderPass.h"
#include "Renderer/Camera.h"

namespace rv {

class ShadowPass : public RenderPass
{
public:
    ~ShadowPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx,RenderFrame& frame) override;

private:
    void InitDirectionalShadowMap();
    void InitPointShadowMap();
    void RenderDirectionalShadowMap(const RenderContext& ctx, RenderFrame& frame);
    void RenderPointShadowMap(const RenderContext& ctx, RenderFrame& frame);

    GLuint fbo = 0;
    GLuint o_DirectionalShadowMap = 0;
    GLuint pointFBO = 0;
    GLuint o_PointShadowMap = 0;
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    const unsigned int POINT_SHADOW_WIDTH = 512, POINT_SHADOW_HEIGHT = 512;
};

}