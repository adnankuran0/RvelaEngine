#pragma once
#include "../RenderPass.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class SSRPass : public RenderPass
{
public:
	const char* GetName() const override { return "SSRPass"; }
    ~SSRPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;

private:
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;
    GLuint o_SsrTexture = 0;
    GLuint ssrFBO = 0;
};

}