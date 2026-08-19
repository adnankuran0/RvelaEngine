#pragma once
#include "../RenderPass.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class ParticlePass : public RenderPass
{
public:
    ~ParticlePass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    GLuint m_InstanceSSBO = 0;
};

}