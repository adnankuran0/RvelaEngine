#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"


class OutlinePass : public RenderPass
{
public:
    ~OutlinePass() {}
    void Execute() override;
    void Init() override {}

    void SetScreenFBO(GLuint screenFBO) { i_ScreenFBO = screenFBO; }
    void SetIntermediateFBO(GLuint intermediateFBO) { i_IntermiedateFBO = intermediateFBO; }
private:
    GLuint i_ScreenFBO;
    GLuint i_IntermiedateFBO;
};
