#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"


class MeshPass : public RenderPass
{
public:
	MeshPass(const RenderContext& context) : RenderPass(context) {}
	void Execute() override;

	GLuint shadowMap = 0;
	glm::mat4 lightSpaceMatrix;
};

