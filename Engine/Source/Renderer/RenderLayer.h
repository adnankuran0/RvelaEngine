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
	[[nodiscard]] inline GLuint GetFinalFramebuffer() { return m_RenderPipeline->GetFinalFramebuffer(); }
	RenderContext& GetRenderContext() { return m_Context; }
	RenderPipeline& GetRenderPipeline() { return *m_RenderPipeline; }
	RenderPassHandle PushRenderPass(std::unique_ptr<RenderPass> pass) { return m_RenderPipeline->PushRenderPass(std::move(pass)); }
	RenderPass* GetRenderPass(RenderPassHandle id) { return m_RenderPipeline->GetRenderPass(id); };

	[[nodiscard]] bool GetWireframe() const noexcept { return m_Wireframe; }
	void SetWireframe(bool enabled) noexcept { m_Wireframe = enabled; }

	void SetDebugBufferMode(DebugBufferMode mode) noexcept { m_RenderPipeline->SetDebugBufferMode(mode); }
	[[nodiscard]] DebugBufferMode GetDebugBufferMode() const noexcept { return m_RenderPipeline->GetDebugBufferMode(); }
	void SetDebugBufferLayer(int layer) noexcept { m_RenderPipeline->SetDebugBufferLayer(layer); }
	[[nodiscard]] int GetDebugBufferLayer() const noexcept { return m_RenderPipeline->GetDebugBufferLayer(); }

	void SetTargetResolution(uint32_t width, uint32_t height) { m_TargetWidth = width; m_TargetHeight = height; }
	[[nodiscard]] uint32_t GetTargetWidth() const noexcept { return m_TargetWidth; }
	[[nodiscard]] uint32_t GetTargetHeight() const noexcept { return m_TargetHeight; }

private:
	Engine* m_Engine;
	std::unique_ptr<RenderPipeline> m_RenderPipeline;
	RenderContext m_Context;
	bool m_Wireframe = false;
	uint32_t m_TargetWidth = 0;
	uint32_t m_TargetHeight = 0;
	void CollectRenderCommands(Scene* scene);

};

}
