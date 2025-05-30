#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
#include <array>


class PostProcessPass : public RenderPass
{
public:
	PostProcessPass(const RenderContext& context) : RenderPass(context)
	{
        if (!isInitialized)
        {
            float quadVertices[] = {
                // positions   // texCoords
                -1.0f,  1.0f,   0.0f, 1.0f,
                -1.0f, -1.0f,   0.0f, 0.0f,
                 1.0f, -1.0f,   1.0f, 0.0f,

                -1.0f,  1.0f,   0.0f, 1.0f,
                 1.0f, -1.0f,   1.0f, 0.0f,
                 1.0f,  1.0f,   1.0f, 1.0f
            };

            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

            isInitialized = true;
        }
        
	}

	void Execute() override;

    GLuint screenTexture;
    GLuint bloomBlurTexture;
    GLuint aoTexture;

private:
    static bool isInitialized;
    static GLuint quadVAO, quadVBO;
};
