#pragma once

class RenderPass
{
public:
	virtual ~RenderPass() = default;
	virtual void Execute() = 0;
};