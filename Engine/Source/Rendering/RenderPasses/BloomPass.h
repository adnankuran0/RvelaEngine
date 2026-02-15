#pragma once
#include "../RenderPass.h"

namespace rv {

class BloomPass : public RenderPass
{
public:
    ~BloomPass();
    void Execute() override;
    void Init() override;
    GLuint GetBloomBlurTexture() { return o_BlurredTexture; }

    void SetBrightTexture(GLuint brightTexture) { i_BrightTexture = brightTexture; }
    

private:
    void Downsample();
    void Upsample();

    GLuint i_BrightTexture = 0;
    GLuint o_BlurredTexture = 0;
    std::vector<GLuint> downsampleFBOs;
    std::vector<GLuint> downsampleTextures;

    std::vector<GLuint> upsampleFBOs;
    std::vector<GLuint> upsampleTextures;
    const int mipLevels = 6;
};

}