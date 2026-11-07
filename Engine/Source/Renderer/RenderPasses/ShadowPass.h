#pragma once
#include "../RenderPass.h"
#include "Renderer/Camera.h"
#include "Renderer/Framebuffer.h"
#include "Renderer/Light.h"
#include "Renderer/RenderTypes.h"
#include <array>

namespace rv {

class ShadowPass : public RenderPass
{
public:
	const char* GetName() const override { return "ShadowPass"; }
    ~ShadowPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    void InvalidateCache();

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

    struct CachedPointShadowState {
        bool valid = false;
        uint32_t lightID = 0;
        glm::vec3 position{ 0.0f };
        float radius = 0.0f;
        float shadowBias = 0.0f;
        bool reverseCullFace = false;
    };

    struct CachedSpotShadowState {
        bool valid = false;
        uint32_t lightID = 0;
        glm::vec3 position{ 0.0f };
        glm::vec3 direction{ 0.0f };
        float radius = 0.0f;
        float innerCutoff = 0.0f;
        float outerCutoff = 0.0f;
        float shadowBias = 0.0f;
        bool reverseCullFace = false;
        glm::mat4 lightSpaceMatrix{ 1.0f };
    };

    struct CachedDirectionalShadowState {
        bool valid = false;
        uint32_t lightID = 0;
        glm::vec3 direction{ 0.0f };
        float shadowBias = 0.0f;
        float normalBias = 0.0f;
        bool reverseCullFace = false;
        glm::mat4 cascadeMatrices[NUM_SHADOW_CASCADES]{};
    };

    std::array<CachedPointShadowState, MaxPointLights> m_CachedPointShadows{};
    std::array<CachedSpotShadowState, MaxSpotLights> m_CachedSpotShadows{};
    CachedDirectionalShadowState m_CachedDirectionalShadow{};
    const void* m_LastScene = nullptr;
};

}
