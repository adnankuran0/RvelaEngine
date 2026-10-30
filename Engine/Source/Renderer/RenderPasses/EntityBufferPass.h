#pragma once
#include "../RenderPass.h"
#include "Renderer/Framebuffer.h"

namespace rv {
class EntityBufferPass : public RenderPass
{
public:
	const char* GetName() const override { return "EntityBufferPass"; }
    ~EntityBufferPass() = default;
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;
private:
    Framebuffer m_Framebuffer;
};
}