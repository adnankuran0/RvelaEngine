#include "rvelapch.h"
#include "RenderLayer.h"

void RenderLayer::OnRender()
{
	auto scene = m_Engine->GetScene();
	auto camera = m_Engine->GetCamera();

	m_RenderPipeline->Clear();

	RenderContext context;
	context.camera = camera;
	context.pointLights = scene->CollectPointLights();
	context.directionalLight = scene->CollectDirectionalLight();

	MeshPass meshPass;
	meshPass.SetContext(context);

	auto view = scene->GetRegistry().view<MeshRendererComponent, MaterialComponent, TransformComponent>();
	for (auto& entity : view)
	{
		RenderCommand command(scene->GetComponent<TransformComponent>(entity),
			scene->GetComponent<MeshRendererComponent>(entity),
			scene->GetComponent<MaterialComponent>(entity));
		meshPass.AddCommand(command);
	}

	m_RenderPipeline->AddPass(&meshPass);

	m_RenderPipeline->Execute();
}
