#pragma once
#include <vector>
#include <memory>
#include "RenderPass.h"

class RenderPipeline
{
public:
	void AddPass(std::shared_ptr<RenderPass> pass)
	{
		passes.push_back(pass);
	}

	void Execute()
	{
		for (auto& pass : passes)
		{
			pass->Execute();
		}
	}
private:
	std::vector <std::shared_ptr<RenderPass>> passes;
};

