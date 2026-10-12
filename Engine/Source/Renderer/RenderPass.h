#pragma once
#include "RenderCommand.h"
#include "RenderFrame.h"

namespace rv {

struct RenderContext;

class RenderPass
{
public:
	virtual ~RenderPass() = default;
	virtual void Execute(const RenderContext& ctx, RenderFrame& frame) = 0;
	virtual void Init(const RenderContext& ctx, RenderFrame& frame) = 0;
	virtual void OnResize(uint32_t width, uint32_t height, RenderFrame& frame) {}
};

}