#pragma once
#include "../RenderPass.h"
#include "../Renderer.h"
#include "../EditorCamera.h"
class SkyboxPass : public RenderPass
{
public:
	~SkyboxPass();
	void Execute() override;
	void Init() override;

	void SetScreenFBO(GLuint screenFBO) { i_ScreenFBO = screenFBO; }
private:
	GLuint i_ScreenFBO = 0;

	Skybox m_Skybox;
};

