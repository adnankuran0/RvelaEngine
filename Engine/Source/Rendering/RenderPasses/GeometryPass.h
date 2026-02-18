#pragma once
#include "Rendering/RenderPass.h"


namespace rv {

class GeometryPass : public RenderPass
{
public:
	~GeometryPass();
	void Execute(const RenderContext& ctx, RenderFrame& frame) override;
	void Init(const RenderContext& ctx, RenderFrame& frame) override;

private:
	GLuint o_Normal = 0;
	GLuint o_Depth = 0;
	GLuint o_Metallic = 0;
	GLuint o_Roughness = 0;
	GLuint gBuffer = 0;
};

}
