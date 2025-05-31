#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
#include <array>


class BrightPass : public RenderPass
{
public:
    ~BrightPass();
    void Execute() override;
    void Init() override;
    GLuint GetBrightTexture() { return o_BrightColorTex; }
    void SetScreenTexture(GLuint screenTexture) { i_ScreenTexture = screenTexture; }

private:
    GLuint i_ScreenTexture = 0;
    GLuint o_BrightColorTex = 0;
    GLuint brightFBO = 0;
};
