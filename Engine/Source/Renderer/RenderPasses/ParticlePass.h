#pragma once
#include "../RenderPass.h"
#include <Renderer/RenderFrame.h>

namespace rv {

class ParticlePass : public RenderPass
{
public:
	const char* GetName() const override { return "ParticlePass"; }
    ~ParticlePass();
    void Init(const RenderContext& ctx, RenderFrame& frame) override;
    void Execute(const RenderContext& ctx, RenderFrame& frame) override;

private:
    GLuint m_InstanceSSBO = 0;
};

}