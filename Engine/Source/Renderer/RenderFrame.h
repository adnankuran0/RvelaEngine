#pragma once
#include "RenderCommand.h"
#include "RenderResource.h"


namespace rv {

struct RenderFrame
{
public:
    std::vector<RenderCommand> opaqueCommands;
    std::vector<RenderCommand> transparentCommands;
    std::vector<ParticleRenderCommand> particleCommands;
    RenderResourceRegistry registry;

private:
    friend class RenderPipeline;
    void Reset()
    {
        opaqueCommands.clear();
        transparentCommands.clear();
        particleCommands.clear();
        //registry.Clear();
    }
};


}
