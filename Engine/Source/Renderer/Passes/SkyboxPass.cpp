#include "rvelapch.h"
#include "SkyboxPass.h"

void SkyboxPass::Init()
{
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

SkyboxPass::~SkyboxPass()
{
	//TODO: Fill this function
}
void SkyboxPass::Execute()
{
	m_Skybox.Render(Renderer::GetSkyboxShader(), ctx.camera->projection, ctx.camera->GetViewMatrix(),screenFBO);
}