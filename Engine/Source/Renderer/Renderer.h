#pragma once

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "BufferLayout.h"
#include "Texture.h"
#include "Shader.h"
#include "EditorCamera.h"
#include "GLFW/glfw3.h"
#include "../Core/Time.h"
#include "Scene/Components.h"
#include "Skybox.h"

struct PointLightData {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
    bool castShadows;
    unsigned int shadowWidth;
    unsigned int shadowHeight;
    GLuint depthCubemap;
    GLuint depthMapFBO;
};

struct DirectionalLightData {
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    bool castShadows;
};



class Renderer
{
public:
    Renderer();
    ~Renderer();
    static void Init(GLFWwindow* window);
    static void StartFrame();
    static void EndFrame();
    static void Shutdown();

    static Shader& GetDefaultShader() { return m_DefaultShader; }
    static Shader& GetSkyboxShader() { return m_SkyboxShader; }
    static Shader& GetShadowShader() { return m_ShadowShader; }
    static Shader& GetPointShadowShader() { return m_PointShadowShader; }
    static Skybox& GetSkybox() { return m_Skybox; }

private:
    static GLFWwindow* activeWindow;
    static Shader m_DefaultShader;
    static Shader m_SkyboxShader;
    static Shader m_ShadowShader;
    static Shader m_PointShadowShader;
    static Skybox m_Skybox;
};