#include "LightInspectors.h"
#include "InspectorUIUtils.h"
#include "ImGui/imgui.h"
#include "Scene/Entity.h"

namespace rv {

void LightInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<CameraComponent>("Camera", registry, selectedEntity, [](CameraComponent& camComp)
	{
		if (UI::BeginPropertyTable("CameraTable"))
		{
			auto& camera = camComp.camera;

			UI::PropertyLabel("Active");
			ImGui::Checkbox("##IsActive", &camComp.isActive);

			UI::PropertyLabel("Projection");
			const char* projectionTypes[] = { "Perspective", "Orthographic" };
			int currentProj = static_cast<int>(camera.ProjectionType);
			if (ImGui::Combo("##Projection", &currentProj, projectionTypes, 2))
				camera.ProjectionType = static_cast<Camera::Projection>(currentProj);

			if (camera.ProjectionType == Camera::Projection::Perspective)
			{
				UI::PropertyLabel("Field of View");
				ImGui::SliderFloat("##FOV", &camera.FOV, 1.0f, 120.0f, "%.1f deg");
			}
			else
			{
				UI::PropertyLabel("Ortho Size");
				ImGui::SliderFloat("##OrthoSize", &camera.OrthoSize, 0.1f, 100.0f, "%.2f");
			}

			UI::PropertyLabel("Near Clip");
			ImGui::SliderFloat("##NearClip", &camera.NearClip, 0.001f, 10.0f, "%.3f");

			UI::PropertyLabel("Far Clip");
			ImGui::SliderFloat("##FarClip", &camera.FarClip, 1.0f, 5000.0f, "%.1f");

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<PointLightComponent>("Point Light", registry, selectedEntity, [](PointLightComponent& pointLight)
	{
		if (UI::BeginPropertyTable("PointLightTable"))
		{
			UI::PropertyLabel("Color");
			float color[3] = { pointLight.color.r, pointLight.color.g, pointLight.color.b };
			if (ImGui::ColorEdit3("##LightColor", color, ImGuiColorEditFlags_NoInputs))
				pointLight.color = glm::vec3(color[0], color[1], color[2]);

			UI::PropertyLabel("Intensity");
			ImGui::DragFloat("##Intensity", &pointLight.intensity, 0.1f, 0.0f, 100.0f, "%.2f");

			UI::PropertyLabel("Radius");
			ImGui::SliderFloat("##Radius", &pointLight.radius, 0.1f, 50.0f, "%.2f");

			UI::PropertyLabel("Falloff");
			ImGui::SliderFloat("##Falloff", &pointLight.falloff, 0.0f, 10.0f, "%.2f");

			UI::PropertyLabel("Cast Shadows");
			ImGui::Checkbox("##CastShadows", &pointLight.castShadows);

			if (pointLight.castShadows)
			{
				UI::PropertyLabel("Shadow Bias");
				ImGui::SliderFloat("##ShadowBias", &pointLight.shadowBias, 0.0f, 0.1f, "%.4f");

				UI::PropertyLabel("Blur Radius");
				ImGui::SliderFloat("##BlurRadius", &pointLight.blurRadius, 0.0f, 0.1f, "%.4f");

				UI::PropertyLabel("Reverse Cull Face");
				ImGui::Checkbox("##ReverseCullFace", &pointLight.reverseCullFace);
			}

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<DirectionalLightComponent>("Directional Light", registry, selectedEntity, [](DirectionalLightComponent& dirLight)
	{
		if (UI::BeginPropertyTable("DirLightTable"))
		{
			UI::PropertyLabel("Color");
			float color[3] = { dirLight.color.r, dirLight.color.g, dirLight.color.b };
			if (ImGui::ColorEdit3("##DirLightColor", color, ImGuiColorEditFlags_NoInputs))
				dirLight.color = glm::vec3(color[0], color[1], color[2]);

			UI::PropertyLabel("Intensity");
			ImGui::DragFloat("##DirIntensity", &dirLight.intensity, 0.05f, 0.0f, 50.0f, "%.2f");

			UI::PropertyLabel("Cast Shadows");
			ImGui::Checkbox("##DirCastShadows", &dirLight.castShadows);

			if (dirLight.castShadows)
			{
				UI::PropertyLabel("Shadow Bias");
				ImGui::SliderFloat("##DirShadowBias", &dirLight.shadowBias, 0.0f, 0.02f, "%.4f");

				UI::PropertyLabel("Normal Bias");
				ImGui::SliderFloat("##DirNormalBias", &dirLight.normalBias, 0.0f, 0.05f, "%.4f");

				UI::PropertyLabel("Blur Radius");
				ImGui::SliderFloat("##DirBlurRadius", &dirLight.blurRadius, 0.0f, 3.0f, "%.3f");

				UI::PropertyLabel("Reverse Cull Face");
				ImGui::Checkbox("##DirReverseCullFace", &dirLight.reverseCullFace);
			}

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<SpotLightComponent>("Spot Light", registry, selectedEntity, [](SpotLightComponent& spotLight)
	{
		if (UI::BeginPropertyTable("SpotLightTable"))
		{
			UI::PropertyLabel("Color");
			float color[3] = { spotLight.color.r, spotLight.color.g, spotLight.color.b };
			if (ImGui::ColorEdit3("##SpotLightColor", color, ImGuiColorEditFlags_NoInputs))
				spotLight.color = glm::vec3(color[0], color[1], color[2]);

			UI::PropertyLabel("Intensity");
			ImGui::DragFloat("##SpotIntensity", &spotLight.intensity, 0.1f, 0.0f, 100.0f, "%.2f");

			UI::PropertyLabel("Radius");
			ImGui::SliderFloat("##SpotRadius", &spotLight.radius, 0.1f, 100.0f, "%.2f");

			UI::PropertyLabel("Inner Angle");
			if (ImGui::SliderFloat("##InnerAngle", &spotLight.innerAngle, 0.1f, 89.0f, "%.1f deg"))
			{
				if (spotLight.innerAngle > spotLight.outerAngle)
					spotLight.outerAngle = spotLight.innerAngle;
				spotLight.UpdateCutoffs();
			}

			UI::PropertyLabel("Outer Angle");
			if (ImGui::SliderFloat("##OuterAngle", &spotLight.outerAngle, 0.1f, 89.0f, "%.1f deg"))
			{
				if (spotLight.outerAngle < spotLight.innerAngle)
					spotLight.innerAngle = spotLight.outerAngle;
				spotLight.UpdateCutoffs();
			}

			UI::PropertyLabel("Cast Shadows");
			ImGui::Checkbox("##SpotCastShadows", &spotLight.castShadows);

			if (spotLight.castShadows)
			{
				UI::PropertyLabel("Shadow Bias");
				ImGui::SliderFloat("##SpotShadowBias", &spotLight.shadowBias, 0.0f, 0.05f, "%.4f");

				UI::PropertyLabel("Blur Radius");
				ImGui::SliderFloat("##SpotBlurRadius", &spotLight.blurRadius, 0.0f, 3.0f, "%.3f");

				UI::PropertyLabel("Reverse Cull Face");
				ImGui::Checkbox("##SpotReverseCullFace", &spotLight.reverseCullFace);
			}

			UI::EndPropertyTable();
		}
	});
}

} // namespace rv
