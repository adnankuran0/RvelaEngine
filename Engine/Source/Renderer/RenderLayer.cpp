#include "rvelapch.h"
#include "RenderLayer.h"

void RenderLayer::OnRender()
{
	auto scene = m_Engine->GetScene();
	auto camera = m_Engine->GetCamera();


	RenderContext context;
	context.camera = camera;
	context.pointLights = scene->CollectPointLights();
	context.directionalLight = scene->CollectDirectionalLight();
	context.viewportWidth = 1920;
	context.viewportHeight = 1080;

	m_RenderPipeline->SetRenderContext(context);
	m_RenderPipeline->EnsureInitialized();

	// Collect commands and submit them to render passes via render pipeline
	CollectRenderCommands(scene, [&](const RenderCommand& cmd) {
		m_RenderPipeline->SubmitRenderCommand(cmd);
		});

	m_RenderPipeline->Execute();
}

void RenderLayer::CollectRenderCommands(Scene* scene, const std::function<void(const RenderCommand&)>& submitCallback)
{
	auto view = scene->GetRegistry().view<TransformComponent, MeshRendererComponent, MaterialComponent>();
	for (auto entity : view)
	{
		RenderCommand cmd(scene->GetComponent<TransformComponent>(entity),
			scene->GetComponent<MeshRendererComponent>(entity),
			scene->GetComponent<MaterialComponent>(entity));

		submitCallback(cmd);
	}
}
