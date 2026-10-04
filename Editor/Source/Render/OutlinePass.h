#pragma once
#include "Renderer/RenderPass.h"

namespace rv {

class OutlinePass : public RenderPass
{
public:
    ~OutlinePass() {}
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

    void SetHasSelection(bool hasSelection) { m_HasSelection = hasSelection; }
    void SetSelectedEntity(entt::entity entity) { m_HasSelection = (entity != entt::null); }
private:
    bool m_HasSelection = false;
    entt::entity m_SelectedEntity = entt::null;
};

}