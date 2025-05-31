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
#include "ScreenQuad.h"

struct PointLightData {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
    float falloff;
    bool castShadows;
    int shadowIndex;
    bool reverseCullFace;
    float blurRadius;
    float shadowBias;
};

struct DirectionalLightData {
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    bool castShadows;
    bool reverseCullFace;
    float blurRadius;
    float shadowBias;
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

    static void DrawFullScreenQuad();

    static Shader& GetPBRShader() { return m_PBRShader; }
    static Shader& GetSkyboxShader() { return m_SkyboxShader; }
    static Shader& GetDirectionalShadowShader() { return m_DirectionalShadowShader; }
    static Shader& GetPointShadowShader() { return m_PointShadowShader; }
    static Shader& GetBrightShader() { return m_BrightShader; }
    static Shader& GetDownsampleShader() { return m_DownsampleShader; }
    static Shader& GetUpsampleShader() { return m_UpsampleShader; }
    static Shader& GetGeometryShader() { return m_GeometryShader; }
    static Shader& GetSSAOShader() { return m_SSAOShader; }
    static Shader& GetSSRShader() { return m_SSRShader; }
    static Shader& GetCompositeShader() { return m_CompositeShader; }

private:
    static GLFWwindow* activeWindow;
    static Shader m_PBRShader;
    static Shader m_SkyboxShader;
    static Shader m_DirectionalShadowShader;
    static Shader m_PointShadowShader;
    static Shader m_BrightShader;
    static Shader m_DownsampleShader;
    static Shader m_UpsampleShader;
    static Shader m_GeometryShader;
    static Shader m_SSAOShader;
    static Shader m_SSRShader;
    static Shader m_CompositeShader;

    static ScreenQuad m_ScreenQuad;
};