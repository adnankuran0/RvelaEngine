#pragma once
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "BufferLayout.h"
#include "Texture.h"
#include "Shader.h"
#include "GLFW/glfw3.h"
#include "Core/Time.h"
#include "Scene/Components.h"
#include "Skybox.h"
#include "ScreenQuad.h"

class ScreenQuad
{
public:
    void Draw()
    {
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void Init()
    {
        //Init screen quad
        float quadVertices[] = {
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
    }
private:
	unsigned int quadVAO;
	unsigned int  quadVBO;
};