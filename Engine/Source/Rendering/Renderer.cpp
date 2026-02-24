#include "rvelapch.h"
#include "Renderer.h"
#include "Core/Log.h"
#include "Core/Time.h"
#include "Input/Input.h"
#include "Utils/FileUtils.h"
#include "ShaderManager.h"

using namespace rv;


ScreenQuad Renderer::m_ScreenQuad;

Renderer::Renderer()
{

};

Renderer::~Renderer()
{
}

void Renderer::Init(GLFWwindow* window)
{
    Renderer::activeWindow = window;
    ShaderManager::Add(Shader("Geometry",VRT_PATH("Assets\\Shaders\\geometry.glsl")));
    ShaderManager::Add(Shader("PointShadow",VRT_PATH("Assets\\Shaders\\pointShadow.glsl")));
    ShaderManager::Add(Shader("DirectionalShadow",VRT_PATH("Assets\\Shaders\\directionalShadow.glsl")));
    ShaderManager::Add(Shader("Skybox",VRT_PATH("Assets\\Shaders\\skybox.glsl")));
    ShaderManager::Add(Shader("PBR",VRT_PATH("Assets\\Shaders\\pbr.glsl")));
    ShaderManager::Add(Shader("Bright",VRT_PATH("Assets\\Shaders\\brightPass.glsl")));
    ShaderManager::Add(Shader("Downsample",VRT_PATH("Assets\\Shaders\\downsample.glsl")));
    ShaderManager::Add(Shader("Upsample",VRT_PATH("Assets\\Shaders\\upsample.glsl")));
    ShaderManager::Add(Shader("SSAO",VRT_PATH("Assets\\Shaders\\ssao.glsl")));
    ShaderManager::Add(Shader("SSR",VRT_PATH("Assets\\Shaders\\ssr.glsl")));
    ShaderManager::Add(Shader("Composite",VRT_PATH("Assets\\Shaders\\composite.glsl")));
    ShaderManager::Add(Shader("ProceduralSky",VRT_PATH("Assets\\Shaders\\proceduralSky.glsl")));
    ShaderManager::Add(Shader("Outline",VRT_PATH("Assets\\Shaders\\outline.glsl")));
    ShaderManager::Add(Shader("ToCubemap",VRT_PATH("Assets\\Shaders\\equirectangularToCubemap.glsl")));
    ShaderManager::Add(Shader("EntityBuffer",VRT_PATH("Assets\\Shaders\\entityBuffer.glsl")));
    ShaderManager::Add(Shader("Mask",VRT_PATH("Assets\\Shaders\\mask.glsl")));
    ShaderManager::Add(Shader("Irradiance",VRT_PATH("Assets\\Shaders\\irradiance.glsl")));
    ShaderManager::Add(Shader("Prefilter",VRT_PATH("Assets\\Shaders\\prefilter.glsl")));
    ShaderManager::Add(Shader("BRDF",VRT_PATH("Assets\\Shaders\\brdf.glsl")));

    m_ScreenQuad.Init();

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
   
}

void Renderer::DrawFullScreenQuad()
{
    m_ScreenQuad.Draw();
}

