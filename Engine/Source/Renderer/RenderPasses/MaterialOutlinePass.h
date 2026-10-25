#pragma once
#include "Renderer/RenderPass.h"
#include "Renderer/FrameBuffer.h"

namespace rv {

class MaterialOutlinePass : public RenderPass
{
public:
    ~MaterialOutlinePass() override = default;
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;

private:
    Framebuffer m_MaskFramebuffer;
};

}
