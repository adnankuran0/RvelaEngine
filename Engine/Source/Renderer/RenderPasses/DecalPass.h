#pragma once
#include "../RenderPass.h"
#include "Renderer/RenderFrame.h"

namespace rv {

class DecalPass : public RenderPass
{
public:
    DecalPass() = default;
    ~DecalPass() override;

    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    GLuint m_CubeVAO = 0;
    GLuint m_CubeVBO = 0;
};

}
