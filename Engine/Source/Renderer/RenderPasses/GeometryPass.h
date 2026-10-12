#pragma once
#include "Renderer/RenderPass.h"
#include "Renderer/FrameBuffer.h"

namespace rv {

class GeometryPass : public RenderPass
{
public:
	~GeometryPass() = default;
	void Execute(const RenderContext& ctx, RenderFrame& frame) override;
	void Init(const RenderContext& ctx, RenderFrame& frame) override;
	void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) override;

private:
	GLuint o_Normal = 0;
	GLuint o_Depth = 0;
	GLuint o_Metallic = 0;
	GLuint o_Roughness = 0;
	Framebuffer m_GBuffer;
};

}
