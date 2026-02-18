#pragma once
#include <memory>
#include "RenderPass.h"
#include "RenderCommand.h"
#include "RenderFrame.h"


namespace rv {

class Engine;
struct RenderContext;
class RenderLayer;

struct RenderFeatures
{
	bool ssao = true;
	bool ssr = true;
	bool bloom = true;
};


class RenderPipeline
{
public:

	RenderPipeline(Engine* engine);
	void EnsureInitialized(const RenderContext& ctx);
	[[nodiscard]] inline GLuint GetFinalTexture() noexcept { return m_RenderFrame.registry.Get("FinalTexture")->id; }
	[[nodiscard]] inline GLuint GetEntityBuffer() noexcept { return m_RenderFrame.registry.Get("EntityBuffer")->id; }
	void Execute(const RenderContext& ctx);

private:
	friend class RenderLayer;

	Engine* engine;
	RenderFrame m_RenderFrame;
	RenderFeatures m_RenderFeatures;

	bool isPipelineDirty = true;
	bool isInitialized = false;
	//Render Passes
	std::vector<RenderPass*> m_RenderPasses;


};

}