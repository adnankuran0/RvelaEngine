#include "../Core/Engine.h"
#include "../Core/Layer.h"
#include "Renderer/Passes/MeshPass.h"
#include "Renderer/Passes/SkyboxPass.h"
#include "Renderer/Passes/ShadowPass.h"
#include "RenderContext.h"
#include "RenderPipeline.h"

class RenderLayer : public Layer
{
public:
	RenderLayer(Engine* engine) : Layer("RenderLayer"), m_Engine(engine) 
	{
		m_RenderPipeline = std::make_unique<RenderPipeline>();
	}

	void OnRender() override;

private:
	Engine* m_Engine;
	std::unique_ptr<RenderPipeline> m_RenderPipeline;

};
