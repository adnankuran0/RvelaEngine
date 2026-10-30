#pragma once
#include "Renderer/RenderPass.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/Framebuffer.h"

namespace rv {

class CompositePass : public RenderPass
{
public:
	const char* GetName() const override { return "CompositePass"; }
    ~CompositePass() override;
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;

private:
    Framebuffer m_Framebuffer;
    Framebuffer m_FXAAFramebuffer;

    GLuint m_AutoExposureSSBO = 0;
    bool m_FirstFrame = true;
};

}
