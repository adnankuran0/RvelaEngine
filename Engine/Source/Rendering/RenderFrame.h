#pragma once
#include "RenderCommand.h"
#include "RenderResource.h"


namespace rv {

struct RenderFrame
{
public:
    std::vector<RenderCommand> commands;
    RenderResourceRegistry registry;

private:
    friend class RenderPipeline;
    void Reset()
    {
        commands.clear();
        //registry.Clear();
    }
};


}
