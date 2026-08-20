#pragma once
#include "Renderer/RenderPass.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/Framebuffer.h"

namespace rv {

class CompositePass : public RenderPass
{
public:
    ~CompositePass() override = default;
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    Framebuffer m_Framebuffer;
};

}