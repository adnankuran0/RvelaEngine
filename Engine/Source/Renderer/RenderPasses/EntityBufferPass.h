#pragma once
#include "../RenderPass.h"
#include "Renderer/Framebuffer.h"

namespace rv {
class EntityBufferPass : public RenderPass
{
public:
    ~EntityBufferPass() = default;
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
private:
    Framebuffer m_Framebuffer;
};
}