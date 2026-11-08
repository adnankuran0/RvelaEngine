#pragma once
#include "RenderCommand.h"
#include "RenderResource.h"


namespace rv {

struct RenderFrame
{
public:
    std::vector<RenderCommand> opaqueCommands;
    std::vector<SkeletalRenderCommand> skeletalOpaqueCommands;
    std::vector<RenderCommand> shadowOpaqueCommands;
    std::vector<SkeletalRenderCommand> shadowSkeletalOpaqueCommands;
    std::vector<RenderCommand> transparentCommands;
    std::vector<SkeletalRenderCommand> skeletalTransparentCommands;
    std::vector<ParticleRenderCommand> particleCommands;
    std::vector<DecalRenderCommand> decalCommands;
    RenderResourceRegistry registry;

private:
    friend class RenderPipeline;
    void Reset()
    {
        opaqueCommands.clear();
        skeletalOpaqueCommands.clear();
        shadowOpaqueCommands.clear();
        shadowSkeletalOpaqueCommands.clear();
        transparentCommands.clear();
        skeletalTransparentCommands.clear();
        particleCommands.clear();
        decalCommands.clear();
        //registry.Clear();
    }
};


}
