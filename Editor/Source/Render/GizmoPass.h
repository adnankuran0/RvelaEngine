#pragma once
#include "Renderer/RenderPass.h"

namespace rv {
    class GizmoPass : public RenderPass {
    public:
        void Init(const RenderContext& ctx, RenderFrame& frame) override;
        void Execute(const RenderContext& ctx, RenderFrame& frame) override;

    private:
        glm::mat4 m_GizmoProj;
    };
}