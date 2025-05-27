#include "rvelapch.h"
#include "Renderer.h"
#include "../RvelaLog.h"
#include "Core/Input/Input.h"
#include "Core/Time.h"
#include "Core/Utils/MaterialManager.h"
#include "Core/Utils/TextureManager.h"
#include "Core/Utils/FileUtils.h"
#include <algorithm>

GLFWwindow* Renderer::activeWindow = nullptr;
Shader Renderer::m_DefaultShader;
Shader Renderer::m_SkyboxShader;
Shader Renderer::m_ShadowShader;
Shader Renderer::m_PointShadowShader;
Skybox Renderer::m_Skybox;



Renderer::Renderer()
{

};

Renderer::~Renderer()
{
    Shutdown();
}

void Renderer::Init(GLFWwindow* window)
{
    Renderer::activeWindow = window;
    m_DefaultShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\pbrVert.glsl"), TO_ABSOLUTE_PATH("Assets\\Shaders\\pbrFrag.glsl"));
    m_SkyboxShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\skyboxVert.glsl"), TO_ABSOLUTE_PATH("Assets\\Shaders\\skyboxFrag.glsl"));
    m_ShadowShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\shadowVert.glsl"), TO_ABSOLUTE_PATH("Assets\\Shaders\\shadowFrag.glsl"));
    m_PointShadowShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\pointShadowVert.glsl"), TO_ABSOLUTE_PATH("Assets\\Shaders\\pointShadowFrag.glsl"), TO_ABSOLUTE_PATH("Assets\\Shaders\\pointShadowGeom.glsl"));

    std::vector<Path> faces = {
    TO_ABSOLUTE_PATH("Assets\\Textures\\skybox\\right.jpg"),
    TO_ABSOLUTE_PATH("Assets\\Textures\\skybox\\left.jpg"),
    TO_ABSOLUTE_PATH("Assets\\Textures\\skybox\\top.jpg"),
    TO_ABSOLUTE_PATH("Assets\\Textures\\skybox\\bottom.jpg"),
    TO_ABSOLUTE_PATH("Assets\\Textures\\skybox\\front.jpg"),
    TO_ABSOLUTE_PATH("Assets\\Textures\\skybox\\back.jpg")
    };
    m_Skybox.Init(faces);
}

void Renderer::StartFrame()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void Renderer::EndFrame()
{

}


void Renderer::Shutdown()
{
    m_DefaultShader.Destroy();
}