#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "Scene/Camera.h"

namespace rv {

class ShadowPass : public RenderPass
{
public:
    ~ShadowPass();
    void Execute() override;
    void Init() override;

    GLuint GetDirectionalShadowMap() const { return o_DirectionalShadowMap; }
    GLuint GetPointShadowMap() const { return o_PointShadowMap; }
    glm::mat4 GetLightSpaceMatrix() const { return o_LightSpaceMatrix; }

private:
    void InitDirectionalShadowMap();
    void InitPointShadowMap();
    void RenderDirectionalShadowMap();
    void RenderPointShadowMap();

    GLuint fbo = 0;
    GLuint o_DirectionalShadowMap = 0;
    GLuint pointFBO = 0;
    GLuint o_PointShadowMap = 0;
    glm::mat4 o_LightSpaceMatrix;
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
    const unsigned int POINT_SHADOW_WIDTH = 512, POINT_SHADOW_HEIGHT = 512;
};

}