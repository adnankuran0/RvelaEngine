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
    static void RenderSkybox(EditorCamera* camera);
    static void Render(TransformComponent& transform,
        MeshRendererComponent& data,
        MaterialComponent& metarial,
        EditorCamera* camera,
        const std::vector<PointLightData>& pointLights,
        const DirectionalLightData* directionalLight
        );
    static void Shutdown();

    static Shader& GetDefaultShader() { return m_DefaultShader; }

private:
    static GLFWwindow* activeWindow;
    static Shader m_DefaultShader;
    static Shader m_SkyboxShader;
    static Skybox m_Skybox;
};