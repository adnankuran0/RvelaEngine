#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"


class LightingPass : public RenderPass
{
public:

    ~LightingPass();
	void Execute() override;
    void Init() override;

	GLuint GetScreenTexture() { return o_IntermediateColorTex; }
    GLuint GetScreenFBO() { return o_ScreenFBO; }
    GLuint GetIntermediateFBO() { return intermediateFBO; }

	void SetDirectionalShadowMap(GLuint directionalShadowMap) { i_DirectionalShadowMap = directionalShadowMap; }
	void SetPointShadowMap(GLuint pointShadowMap) { i_PointShadowMap = pointShadowMap; }
	void SetLightSpaceMatrix(glm::mat4 lightSpaceMatrix) { i_LightSpaceMatrix = lightSpaceMatrix; }

private:
	GLuint i_DirectionalShadowMap = 0;
	GLuint i_PointShadowMap = 0;
	glm::mat4 i_LightSpaceMatrix;
	GLuint o_ScreenFBO = 0;
	GLuint o_IntermediateColorTex = 0;

	GLuint screenColorTex = 0;
	GLuint screenRBO = 0;
	GLuint intermediateFBO = 0;
};
