#pragma once
#include "../RenderPass.h"

namespace rv {

class OutlinePass : public RenderPass
{
public:
    ~OutlinePass() {}
    void Init(const RenderContext& ctx, RenderFrame& frame) override {}
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

    
};

}