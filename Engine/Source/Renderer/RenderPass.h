#pragma once
#include "RenderContext.h"
#include "RenderCommand.h"

class RenderPass
{
public:
	virtual ~RenderPass() = default;
	virtual void Execute() = 0;

	void SetContext(const RenderContext& ctx) { this->ctx = ctx; }
	RenderContext& GetContext() { return ctx; }
	void AddCommand(const RenderCommand& cmd) { commands.push_back(cmd); }

protected:
	RenderContext ctx;
	std::vector<RenderCommand> commands;
};