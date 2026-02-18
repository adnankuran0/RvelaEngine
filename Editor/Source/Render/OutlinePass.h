#pragma once
#include "Rendering/RenderPass.h"

namespace rv {

class OutlinePass : public RenderPass
{
public:
    ~OutlinePass() {}
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

    void SetSelectedEntity(entt::entity entity) { m_SelectedEntity = entity; }
private:
    entt::entity m_SelectedEntity;
    
};

}