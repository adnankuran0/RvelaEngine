#pragma once
#include "../RenderPass.h"
#include "Renderer/FrameBuffer.h"

namespace rv {

class RenderPipeline;

class DebugBufferPass : public RenderPass
{
public:
	const char* GetName() const override { return "DebugBufferPass"; }
    explicit DebugBufferPass(RenderPipeline* pipeline);
    ~DebugBufferPass() override;

    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    void CreateDummyTextures();
    void DestroyDummyTextures();

    RenderPipeline* m_Pipeline = nullptr;
    Framebuffer m_Framebuffer;
    GLuint m_DummyTex2D = 0;
    GLuint m_DummyTexArray = 0;
    GLuint m_DummyTexUInt = 0;
    GLuint m_DummyTexCubeArray = 0;
};

}
