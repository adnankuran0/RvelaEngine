#pragma once
#include "../RenderPass.h"

namespace rv {

class EntityBufferPass : public RenderPass
{
public:

    ~EntityBufferPass() {}
    void Execute() override;
    void Init() override;
private:
    GLuint m_Framebuffer = 0;
    GLuint o_EntityTexture = 0;
 
};

}