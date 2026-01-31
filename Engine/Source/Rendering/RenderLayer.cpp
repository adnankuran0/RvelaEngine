#include "rvelapch.h"
#include "RenderLayer.h"

void RenderLayer::OnRender()
{
	Scene& scene = m_Engine->GetActiveScene();
	ICamera* camera = m_Engine->GetCamera();

	RenderContext context;
	
	context.camera = camera;
	context.pointLights = scene.CollectPointLights();
	context.directionalLight = scene.CollectDirectionalLight();
	context.viewportWidth = 1920;
	context.viewportHeight = 1080;
	context.scene = &scene;
	m_RenderPipeline->SetRenderContext(context);
	m_RenderPipeline->EnsureInitialized();

	// Collect commands and submit them to render passes via render pipeline
	CollectRenderCommands(&scene, [&](const RenderCommand& cmd) {
		m_RenderPipeline->SubmitRenderCommand(cmd);
		});

	m_RenderPipeline->Execute();
}

void RenderLayer::CollectRenderCommands(Scene* scene, const std::function<void(const RenderCommand&)>& submitCallback)
{
	auto view = scene->GetRegistry().view<TransformComponent, MeshComponent ,MeshRendererComponent, MaterialComponent>();
	for (auto entity : view)
	{
		

		MeshComponent& meshComp = scene->GetComponent<MeshComponent>(entity);
		
		if (meshComp.IsDirty())
		{
			scene->GetComponent<MeshRendererComponent>(entity).RecreateFromMesh(meshComp.GetMesh());
			meshComp.SetDirty(false);
		}

		if (entity == scene->GetSelectedEntity())
		{
			RenderCommand cmd(scene->GetComponent<TransformComponent>(entity),
				scene->GetComponent<MeshRendererComponent>(entity),
				scene->GetComponent<MaterialComponent>(entity),
				true);
			submitCallback(cmd);
		}
		else
		{
			RenderCommand cmd(scene->GetComponent<TransformComponent>(entity),
				scene->GetComponent<MeshRendererComponent>(entity),
				scene->GetComponent<MaterialComponent>(entity),
				false);
			submitCallback(cmd);
		}
	}
}
