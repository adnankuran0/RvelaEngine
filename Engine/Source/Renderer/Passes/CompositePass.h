#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
#include <array>


class CompositePass : public RenderPass
{
public:
    CompositePass(const RenderContext& context) : RenderPass(context), screenTexture(-1), bloomBlurTexture(-1), aoTexture(-1), ssrTexture(-1)
	{
        if (!isInitialized)
        {
           

            isInitialized = true;
        }
        
	}

	void Execute() override;

    GLuint screenTexture;
    GLuint bloomBlurTexture;
    GLuint aoTexture;
    GLuint ssrTexture;

private:
    static bool isInitialized;
    static float exposure;
    void UpdateExposure(float deltaTime);
};
