#pragma once

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "BufferLayout.h"
#include "Texture.h"
#include "Shader.h"
#include "Camera.h"
#include "GLFW/glfw3.h"
#include "../Core/Time.h"
#include "Scene/Components.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();
    static void Init(GLFWwindow* window);
    static void StartFrame();
    static void EndFrame();
    static void Render(WorldTransformComponent& transform,MeshComponent& data, MaterialComponent& metarial, Camera& camera);
    static void Shutdown();

private:
    static GLFWwindow* activeWindow;
    static Shader m_DefaultShader;
};