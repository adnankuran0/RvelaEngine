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
#include "ShaderManager.h"

namespace rv {

class Renderer
{
public:
    Renderer();
    ~Renderer();
    void Init(GLFWwindow* window);
    void StartFrame();
    void EndFrame();
    void Shutdown();

    static void DrawFullScreenQuad();
    

private:
    inline static GLFWwindow* activeWindow = nullptr;
   

    static ScreenQuad m_ScreenQuad;
};

}