#include "Core/Layer.h"
#include "RenderContext.h"
#include "RenderPipeline.h"

namespace rv {

class Engine;
class Scene;

class RenderLayer : public Layer
{
public:
	RenderLayer(Engine* engine) : Layer("RenderLayer"), m_Engine(engine) 
	{
		m_RenderPipeline = std::make_unique<RenderPipeline>();
	}

	void OnRender() override;

	GLuint GetEntityBuffer() { return m_RenderPipeline->GetEntityBuffer(); }
	[[nodiscard]] inline GLuint GetFinalTexture() noexcept { return m_RenderPipeline->GetFinalTexture(); }
	RenderContext& GetRenderContext() { return m_Context; }
	RenderPassHandle PushRenderPass(std::unique_ptr<RenderPass> pass) { return m_RenderPipeline->PushRenderPass(std::move(pass)); }
	RenderPass* GetRenderPass(RenderPassHandle id) { return m_RenderPipeline->GetRenderPass(id); };

private:
	Engine* m_Engine;
	std::unique_ptr<RenderPipeline> m_RenderPipeline;
	RenderContext m_Context;
	void CollectRenderCommands(Scene* scene);

};

}
