#pragma once
#include "../RenderPass.h"
#include "Renderer/Camera.h"
#include "Renderer/Framebuffer.h"
#include <Renderer/RenderFrame.h>

namespace rv {
class LightingPass : public RenderPass
{
public:
    ~LightingPass() = default;
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
private:
    Framebuffer m_ScreenFramebuffer;
    Framebuffer m_IntermediateFramebuffer;
};
}