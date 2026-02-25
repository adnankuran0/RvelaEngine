#pragma once
#include "../RenderPass.h"
#include <Rendering/RenderFrame.h>

namespace rv {

class BloomPass : public RenderPass
{
public:
    ~BloomPass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;
    

private:
    void Downsample(const RenderContext& ctx, RenderFrame& frame);
    void Upsample(const RenderContext& ctx, RenderFrame& frame);

    GLuint o_BlurredTexture = 0;
    std::vector<GLuint> downsampleFBOs;
    std::vector<GLuint> downsampleTextures;

    std::vector<GLuint> upsampleFBOs;
    std::vector<GLuint> upsampleTextures;
    const int mipLevels = 8;
};

}