#include "rvelapch.h"
#include "Renderer.h"
#include "Core/Log.h"
#include "Core/Time.h"
#include "Input/Input.h"
#include "Utils/MaterialManager.h"
#include "Utils/TextureManager.h"
#include "Utils/FileUtils.h"


GLFWwindow* Renderer::activeWindow = nullptr;
Shader Renderer::m_DirectionalShadowShader;
Shader Renderer::m_PointShadowShader;
Shader Renderer::m_GeometryShader;
Shader Renderer::m_SkyboxShader;
Shader Renderer::m_PBRShader;
Shader Renderer::m_BrightShader;
Shader Renderer::m_DownsampleShader;
Shader Renderer::m_UpsampleShader;
Shader Renderer::m_SSAOShader;
Shader Renderer::m_SSRShader;
Shader Renderer::m_CompositeShader;
Shader Renderer::m_LuminanceShader;
Shader Renderer::m_ProceduralSkyShader;
Shader Renderer::m_OutlineShader;

ScreenQuad Renderer::m_ScreenQuad;

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
    m_GeometryShader.Init(VRT_PATH("Assets\\Shaders\\geometry.glsl"));
    m_PointShadowShader.Init(VRT_PATH("Assets\\Shaders\\pointShadow.glsl"));
    m_DirectionalShadowShader.Init(VRT_PATH("Assets\\Shaders\\directionalShadow.glsl"));
    m_SkyboxShader.Init(VRT_PATH("Assets\\Shaders\\skybox.glsl"));
    m_PBRShader.Init(VRT_PATH("Assets\\Shaders\\pbr.glsl"));
    m_BrightShader.Init(VRT_PATH("Assets\\Shaders\\brightPass.glsl"));
    m_DownsampleShader.Init(VRT_PATH("Assets\\Shaders\\downsample.glsl"));
    m_UpsampleShader.Init(VRT_PATH("Assets\\Shaders\\upsample.glsl"));
    m_SSAOShader.Init(VRT_PATH("Assets\\Shaders\\ssao.glsl"));
    m_SSRShader.Init(VRT_PATH("Assets\\Shaders\\ssr.glsl"));
    m_CompositeShader.Init(VRT_PATH("Assets\\Shaders\\composite.glsl"));
    m_LuminanceShader.Init(VRT_PATH("Assets\\Shaders\\luminance.glsl"));
    m_ProceduralSkyShader.Init(VRT_PATH("Assets\\Shaders\\proceduralSky.glsl"));
    m_OutlineShader.Init(VRT_PATH("Assets\\Shaders\\outline.glsl"));

    

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
    m_GeometryShader.Destroy();
    m_DirectionalShadowShader.Destroy();
    m_PointShadowShader.Destroy();
    m_SkyboxShader.Destroy();
    m_PBRShader.Destroy();
    m_BrightShader.Destroy();
    m_DownsampleShader.Destroy();
    m_UpsampleShader.Destroy();
    m_SSAOShader.Destroy();
    m_SSRShader.Destroy();
    m_CompositeShader.Destroy();
}

void Renderer::DrawFullScreenQuad()
{
    m_ScreenQuad.Draw();
}
