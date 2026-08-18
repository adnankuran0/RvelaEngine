#pragma once
#include <memory>
#include "RenderPass.h"
#include "RenderCommand.h"
#include "RenderFrame.h"
#include "UniformBuffer.h"

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
	void Execute(const RenderContext& ctx);

	[[nodiscard]] inline GLuint GetFinalTexture() noexcept { return m_RenderFrame.registry.Get("FinalTexture")->id; }
	[[nodiscard]] inline GLuint GetEntityBuffer() noexcept { return m_RenderFrame.registry.Get("EntityBuffer")->id; }

private:
	void UpdateUBOs(const RenderContext& ctx);

	friend class RenderLayer;

	RenderFrame m_RenderFrame;

	bool isPipelineDirty = true;
	bool isInitialized = false;
	std::vector<PassEntry> m_RenderPasses;
	RenderPassHandle m_NextID = 1;

	std::unique_ptr<UniformBuffer> m_CameraUBO;
	std::unique_ptr<UniformBuffer> m_LightUBO;
};

}