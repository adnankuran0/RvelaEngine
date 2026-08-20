#pragma once
#include "Renderer/RenderPass.h"
#include "Renderer/RenderFrame.h"
#include "Renderer/Framebuffer.h"
#include <vector>

namespace rv {

class BloomPass : public RenderPass
{
public:
    ~BloomPass() override = default;
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    void Downsample(const RenderContext& ctx, RenderFrame& frame);
    void Upsample(const RenderContext& ctx, RenderFrame& frame);

private:
    std::vector<Framebuffer> m_DownsampleFBOs;
    static constexpr int s_MipLevels = 8;
};

}