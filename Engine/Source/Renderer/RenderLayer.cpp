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
	context.viewportWidth = m_Engine->GetWindow()->GetSize().width;
	context.viewportHeight = m_Engine->GetWindow()->GetSize().height;

	GeometryPass geometryPass(context);
	ShadowPass shadowPass(context);
	SkyboxPass skyboxPass(context);
	LightingPass lightingPass(context);
	BrightPass brightPass(context);
	BloomPass bloomPass(context);
	SSAOPass ssaoPass(context);
	SSRPass ssrPass(context);
	CompositePass compositePass(context);




	auto view = scene->GetRegistry().view<MeshRendererComponent, MaterialComponent, TransformComponent>();
	for (auto& entity : view)
	{
		RenderCommand command(scene->GetComponent<TransformComponent>(entity),
			scene->GetComponent<MeshRendererComponent>(entity),
			scene->GetComponent<MaterialComponent>(entity));

		shadowPass.AddCommand(command);
		geometryPass.AddCommand(command);
		lightingPass.AddCommand(command);
	}

	m_RenderPipeline->SetGeometryPass(&geometryPass);
	m_RenderPipeline->SetShadowPass(&shadowPass) ;
	m_RenderPipeline->SetSkyboxPass(&skyboxPass);
	m_RenderPipeline->SetLightingPass(&lightingPass);
	m_RenderPipeline->SetBrightPass(&brightPass);
	m_RenderPipeline->SetBloomPass(&bloomPass);
	m_RenderPipeline->SetSSAOPass(&ssaoPass);
	m_RenderPipeline->SetSSRPass(&ssrPass);
	m_RenderPipeline->SetCompositePass(&compositePass);

	m_RenderPipeline->Execute();
}