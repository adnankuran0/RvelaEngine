#pragma once
#include "../RenderPass.h"
#include <Renderer/RenderFrame.h>

namespace rv {

    class TransparentPass : public RenderPass
    {
    public:
        ~TransparentPass();
        void Init(const RenderContext& ctx, RenderFrame& frame) override;
        void Execute(const RenderContext& ctx, RenderFrame& frame) override;


    };

}