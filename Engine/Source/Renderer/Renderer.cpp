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
    m_GeometryShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\geometry.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\geometry.frag"));
    m_PointShadowShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\pointShadow.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\pointShadow.frag"));
    m_DirectionalShadowShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\directionalShadow.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\directionalShadow.frag"));
    m_SkyboxShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\skybox.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\skybox.frag"));
    m_PBRShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\pbr.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\pbr.frag"));
    m_BrightShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\brightPass.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\brightPass.frag"));
    m_DownsampleShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\downsample.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\downsample.frag"));
    m_UpsampleShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\upsample.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\upsample.frag"));
    m_SSAOShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\ssao.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\ssao.frag"));
    m_SSRShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\ssr.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\ssr.frag"));
    m_CompositeShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\composite.vert"), TO_ABSOLUTE_PATH("Assets\\Shaders\\composite.frag"));
    m_LuminanceShader.Init(TO_ABSOLUTE_PATH("Assets\\Shaders\\luminance.comp"));

    

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
