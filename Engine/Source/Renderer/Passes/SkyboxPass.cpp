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
    glBindFramebuffer(GL_FRAMEBUFFER, screenFBO);
    Shader& proceduralSkyShader = Renderer::GetProceduralSkyShader();
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    proceduralSkyShader.use();
    proceduralSkyShader.setMat4("invProjection", glm::inverse(ctx.camera->projection));
    proceduralSkyShader.setMat4("invView", glm::inverse(ctx.camera->GetViewMatrix()));
    proceduralSkyShader.setVec3("lightDirection", ctx.directionalLight->direction);

    Renderer::DrawFullScreenQuad();
   

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
   
   
	//m_Skybox.Render(Renderer::GetSkyboxShader(), ctx.camera->projection, ctx.camera->GetViewMatrix(),screenFBO);
}