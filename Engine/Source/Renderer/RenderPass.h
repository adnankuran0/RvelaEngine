#pragma once
#include "RenderContext.h"
#include "RenderCommand.h"

class RenderPass
{
public:
	void SetRenderContext(const RenderContext& context) { ctx = context; }
	virtual ~RenderPass() = default;
	virtual void Execute() = 0;
	virtual void Init() = 0;

	void AddRenderCommand(const RenderCommand& cmd) { commands.push_back(cmd); }

protected:
	RenderContext ctx;
	std::vector<RenderCommand> commands;
};