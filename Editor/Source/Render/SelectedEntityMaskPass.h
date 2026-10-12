#pragma once
#include "Renderer/RenderPass.h"

namespace rv {

class SelectedEntityMaskPass : public RenderPass {
public:
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;

    void SetSelectedEntities(const std::vector<entt::entity>& entities) { m_SelectedEntities = entities; }
    void SetSelectedEntity(entt::entity entity) {
        if (entity == entt::null) m_SelectedEntities.clear();
        else m_SelectedEntities = { entity };
    }

private:
    std::vector<entt::entity> m_SelectedEntities;
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;
    GLuint m_Framebuffer = 0;
    GLuint o_MaskTexture = 0; 
};

}