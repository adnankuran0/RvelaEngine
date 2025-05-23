#pragma once
#include "RenderContext.h"
#include "RenderCommand.h"

class RenderPass
{
public:
	RenderPass(const RenderContext& context) : ctx(context) {}
	virtual ~RenderPass() = default;
	virtual void Execute() = 0;

	void AddCommand(const RenderCommand& cmd) { commands.push_back(cmd); }

protected:
	RenderContext ctx;
	std::vector<RenderCommand> commands;
};