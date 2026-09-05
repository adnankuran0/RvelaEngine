#pragma once
#include "Renderer/RenderPass.h"
#include "Renderer/Texture.h"
#include <glm/glm.hpp>

namespace rv {

enum class GizmoPriority : int
{
    None = 0,
    Audio = 1,
    Particle = 2,
    Light = 3,
    Camera = 4
};

struct GizmoInfo
{
    uint32_t textureID = 0;
    GizmoPriority priority = GizmoPriority::None;
};

class GizmoPass : public RenderPass
{
public:
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

    GizmoInfo GetEntityPrimaryIcon(entt::registry& reg, entt::entity e);

    glm::mat4 CalculateBillboardTransform(const glm::vec3& worldPos, const glm::vec3& camPos, const glm::mat4& viewMat);

private:
    void DrawCameraFrustum(const RenderContext& ctx, RenderFrame& frame);
    void DrawDirectionalLightArrow(const RenderContext& ctx, RenderFrame& frame);


    void DrawEditorIconsVisual(const RenderContext& ctx, RenderFrame& frame);
    void DrawEditorIconsPicking(const RenderContext& ctx, RenderFrame& frame);

    const float m_GizmoScale = 0.05f;

    Texture m_AudioEmitterIcon;
    Texture m_CameraIcon;
    Texture m_DirectionalLightIcon;
    Texture m_PointLightIcon;
    Texture m_SpotLightIcon;
    Texture m_ParticleEmitterIcon;

    unsigned int m_QuadVAO;
    unsigned int m_QuadVBO;

    glm::mat4 m_CameraFrustumProj;
};

}