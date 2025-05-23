#pragma once
#include "../RenderPass.h"
#include "Scene/Components.h"
#include "../Renderer.h"
#include "../EditorCamera.h"


class MeshPass : public RenderPass
{
public:
	void Execute() override;
};

