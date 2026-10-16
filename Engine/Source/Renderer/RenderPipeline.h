#pragma once
#include <memory>
#include "RenderPass.h"
#include "RenderCommand.h"
#include "RenderFrame.h"
#include "UniformBuffer.h"
#include "RenderTypes.h"

namespace rv {

class Engine;
struct RenderContext;
class RenderLayer;

using RenderPassHandle = uint32_t;

struct PassEntry
{
	RenderPassHandle id;
	std::unique_ptr<RenderPass> pass;
};

class RenderPipeline
{
public:
	RenderPipeline();
	void EnsureInitialized(const RenderContext& ctx);
	RenderPassHandle PushRenderPass(std::unique_ptr<RenderPass> pass);
	RenderPass* GetRenderPass(RenderPassHandle id);
	void OnResize(uint32_t width, uint32_t height);
	void Execute(const RenderContext& ctx);

	[[nodiscard]] inline GLuint GetFinalTexture() noexcept
	{
		if (m_DebugBufferMode != DebugBufferMode::None)
		{
			auto r = m_RenderFrame.registry.Get("DebugBufferTexture");
			if (r && r->id != 0)
				return r->id;
		}
		auto r = m_RenderFrame.registry.Get("FinalTexture");
		return r ? r->id : 0;
	}
	[[nodiscard]] inline GLuint GetFinalFramebuffer() noexcept { auto r = m_RenderFrame.registry.Get("FinalFramebuffer"); return r ? r->id : 0; }
	[[nodiscard]] inline GLuint GetEntityBuffer() noexcept { auto r = m_RenderFrame.registry.Get("EntityBuffer"); return r ? r->id : 0; }
	[[nodiscard]] inline GLuint GetDepthTexture() noexcept { auto r = m_RenderFrame.registry.Get("DepthTexture"); return r ? r->id : 0; }

	void SetDebugBufferMode(DebugBufferMode mode) noexcept { m_DebugBufferMode = mode; }
	[[nodiscard]] DebugBufferMode GetDebugBufferMode() const noexcept { return m_DebugBufferMode; }
	void SetDebugBufferLayer(int layer) noexcept { m_DebugBufferLayer = layer; }
	[[nodiscard]] int GetDebugBufferLayer() const noexcept { return m_DebugBufferLayer; }

private:
	void UpdateUBOs(const RenderContext& ctx);

	friend class RenderLayer;

	RenderFrame m_RenderFrame;

	bool isPipelineDirty = true;
	bool isInitialized = false;
	uint32_t m_CurrentWidth = 0;
	uint32_t m_CurrentHeight = 0;
	std::vector<PassEntry> m_RenderPasses;
	RenderPassHandle m_NextID = 1;

	std::unique_ptr<UniformBuffer> m_CameraUBO;
	std::unique_ptr<UniformBuffer> m_LightUBO;

	DebugBufferMode m_DebugBufferMode = DebugBufferMode::None;
	int m_DebugBufferLayer = 0;
};

}