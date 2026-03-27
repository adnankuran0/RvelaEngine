#pragma once
#include "../RenderPass.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class DebugPass : public RenderPass
{
public:
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
private:
    void DrawAABBs(const RenderContext& ctx);
};

}