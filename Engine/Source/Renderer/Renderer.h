#pragma once

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "BufferLayout.h"
#include "Shader.h"
#include "Camera.h"
#include "GLFW/glfw3.h"

struct RendererData
{
    VertexArray* m_VAO;
    VertexBuffer* m_VBO;
    ElementBuffer* m_EBO;
    BufferLayout* m_Layout;
    Shader* m_Shader;
    Camera* m_Camera;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    static void Init();
    static void Render(GLFWwindow* window);
    static void Shutdown();

    static RendererData& GetData();


private:
    static RendererData s_Data;
    
    static void LoadShaders();
    static void SetupBuffers();
};