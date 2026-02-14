#include "Core/Layer.h"
#include "RenderContext.h"
#include "RenderPipeline.h"

class Engine;
class Scene;

class RenderLayer : public Layer
{
public:
	RenderLayer(Engine* engine) : Layer("RenderLayer"), m_Engine(engine) 
	{
		m_RenderPipeline = std::make_unique<RenderPipeline>(engine);
	}

	void OnRender() override;

private:
	Engine* m_Engine;
	std::unique_ptr<RenderPipeline> m_RenderPipeline;

	void CollectRenderCommands(Scene* scene, const std::function<void(const RenderCommand&)>& submitCallback);

};
