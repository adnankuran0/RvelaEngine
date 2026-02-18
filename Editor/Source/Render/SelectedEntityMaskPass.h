#pragma once
#include "Rendering/RenderPass.h"

namespace rv {

class SelectedEntityMaskPass : public RenderPass {
public:
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

    void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }

private:
    entt::entity m_SelectedEntity = entt::null;
    GLuint m_Framebuffer = 0;
    GLuint o_MaskTexture = 0; 
};

}