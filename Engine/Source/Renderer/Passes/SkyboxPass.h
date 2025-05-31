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

	GLuint screenFBO = 0;
private:
	Skybox m_Skybox;
};

