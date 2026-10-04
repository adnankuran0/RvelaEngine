#pragma once
#include "Renderer/RenderPass.h"

namespace rv {

class SelectedEntityMaskPass : public RenderPass {
public:
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

    void SetSelectedEntities(const std::vector<entt::entity>& entities) { m_SelectedEntities = entities; }
    void SetSelectedEntity(entt::entity entity) {
        if (entity == entt::null) m_SelectedEntities.clear();
        else m_SelectedEntities = { entity };
    }

private:
    std::vector<entt::entity> m_SelectedEntities;
    GLuint m_Framebuffer = 0;
    GLuint o_MaskTexture = 0; 
};

}