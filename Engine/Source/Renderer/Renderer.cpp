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
    ShaderManager::Add(Shader("Geometry",ENGINE_PATH("Shaders\\geometry.glsl")));
    ShaderManager::Add(Shader("PointShadow",ENGINE_PATH("Shaders\\pointShadow.glsl")));
    ShaderManager::Add(Shader("DirectionalShadow",ENGINE_PATH("Shaders\\directionalShadow.glsl")));
    ShaderManager::Add(Shader("Skybox",ENGINE_PATH("Shaders\\skybox.glsl")));
    ShaderManager::Add(Shader("PBR",ENGINE_PATH("Shaders\\pbr.glsl")));
    ShaderManager::Add(Shader("Bright",ENGINE_PATH("Shaders\\brightPass.glsl")));
    ShaderManager::Add(Shader("Downsample",ENGINE_PATH("Shaders\\downsample.glsl")));
    ShaderManager::Add(Shader("Upsample",ENGINE_PATH("Shaders\\upsample.glsl")));
    ShaderManager::Add(Shader("SSAO",ENGINE_PATH("Shaders\\ssao.glsl")));
    ShaderManager::Add(Shader("SSR",ENGINE_PATH("Shaders\\ssr.glsl")));
    ShaderManager::Add(Shader("Composite",ENGINE_PATH("Shaders\\composite.glsl")));
    ShaderManager::Add(Shader("Outline",ENGINE_PATH("Shaders\\outline.glsl")));
    ShaderManager::Add(Shader("ToCubemap",ENGINE_PATH("Shaders\\equirectangularToCubemap.glsl")));
    ShaderManager::Add(Shader("EntityBuffer",ENGINE_PATH("Shaders\\entityBuffer.glsl")));
    ShaderManager::Add(Shader("Mask",ENGINE_PATH("Shaders\\mask.glsl")));
    ShaderManager::Add(Shader("Irradiance",ENGINE_PATH("Shaders\\irradiance.glsl")));
    ShaderManager::Add(Shader("Prefilter",ENGINE_PATH("Shaders\\prefilter.glsl")));
    ShaderManager::Add(Shader("BRDF",ENGINE_PATH("Shaders\\brdf.glsl")));
    ShaderManager::Add(Shader("Line",ENGINE_PATH("Shaders\\line.glsl")));
    ShaderManager::Add(Shader("Particle",ENGINE_PATH("Shaders\\particle.glsl")));

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

