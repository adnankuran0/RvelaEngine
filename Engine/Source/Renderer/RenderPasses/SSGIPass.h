#pragma once
#include "../RenderPass.h"
#include "Renderer/Framebuffer.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class SSGIPass : public RenderPass
{
public:
    ~SSGIPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    Framebuffer m_SSGIFBO;
};

}
