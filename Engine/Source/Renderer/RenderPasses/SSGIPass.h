#pragma once
#include "../RenderPass.h"
#include "Renderer/Framebuffer.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class SSGIPass : public RenderPass
{
public:
	const char* GetName() const override { return "SSGIPass"; }
    ~SSGIPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;

private:
    Framebuffer m_SSGIFBO;
};

}
