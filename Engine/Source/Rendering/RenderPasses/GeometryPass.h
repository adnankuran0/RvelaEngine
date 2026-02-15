#pragma once
#include "Rendering/RenderPass.h"
#include "Rendering/Renderer.h"

namespace rv {

class GeometryPass : public RenderPass
{
public:
	~GeometryPass();
	void Execute() override;
	void Init() override;

	GLuint GetNormalTexure() { return o_Normal; }
	GLuint GetDepthTexure() { return o_Depth; }
	GLuint GetMetallicTexure() { return o_Metallic; }
	GLuint GetRoughnessTexure() { return o_Roughness; }

private:
	GLuint o_Normal = 0;
	GLuint o_Depth = 0;
	GLuint o_Metallic = 0;
	GLuint o_Roughness = 0;
	GLuint gBuffer = 0;
};

}
