#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
#include <array>


class CompositePass : public RenderPass
{
public:

    ~CompositePass() {}
	void Execute() override;
    void Init() override {}

    void SetScreenTexture(GLuint screenTexture) { i_ScreenTexture = screenTexture; }
    void SetBloomBlurTexture(GLuint bloomBlurTexture) { i_BloomBlurTexture = bloomBlurTexture; }
    void SetAoTexture(GLuint aoTexture) { i_AoTexture = aoTexture; }
    void SetSsrTexture(GLuint ssrTexture) { i_SsrTexture = ssrTexture; }

private:
    GLuint i_ScreenTexture = 0;
    GLuint i_BloomBlurTexture = 0;
    GLuint i_AoTexture = 0;
    GLuint i_SsrTexture = 0;
    float exposure = 1.0f;
    void UpdateExposure(float deltaTime);
};
