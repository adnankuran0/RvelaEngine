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
			if (ImGui::SliderFloat("##Radius", &pointLight.radius, 0.1f, 50.0f, "%.2f"))
				pointLight.shadowDirty = true;

			UI::PropertyLabel("Falloff");
			ImGui::SliderFloat("##Falloff", &pointLight.falloff, 0.0f, 10.0f, "%.2f");

			UI::PropertyLabel("Cast Shadows");
			if (ImGui::Checkbox("##CastShadows", &pointLight.castShadows))
				pointLight.shadowDirty = true;

			if (pointLight.castShadows)
			{
				UI::PropertyLabel("Static Shadow");
				if (ImGui::Checkbox("##StaticShadow", &pointLight.staticShadow))
					pointLight.shadowDirty = true;

				if (pointLight.staticShadow)
				{
					UI::PropertyLabel("Update Shadow");
					if (ImGui::Button("Bake##PointShadow"))
						pointLight.shadowDirty = true;
				}

				UI::PropertyLabel("Shadow Bias");
				if (ImGui::SliderFloat("##ShadowBias", &pointLight.shadowBias, 0.0f, 0.1f, "%.4f"))
					pointLight.shadowDirty = true;

				UI::PropertyLabel("Blur Radius");
				ImGui::SliderFloat("##BlurRadius", &pointLight.blurRadius, 0.0f, 0.1f, "%.4f");

				UI::PropertyLabel("Reverse Cull Face");
				if (ImGui::Checkbox("##ReverseCullFace", &pointLight.reverseCullFace))
					pointLight.shadowDirty = true;
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
			if (ImGui::Checkbox("##DirCastShadows", &dirLight.castShadows))
				dirLight.shadowDirty = true;

			if (dirLight.castShadows)
			{
				UI::PropertyLabel("Static Shadow");
				if (ImGui::Checkbox("##DirStaticShadow", &dirLight.staticShadow))
					dirLight.shadowDirty = true;

				if (dirLight.staticShadow)
				{
					UI::PropertyLabel("Update Shadow");
					if (ImGui::Button("Bake##DirShadow"))
						dirLight.shadowDirty = true;
				}

				UI::PropertyLabel("Shadow Bias");
				if (ImGui::SliderFloat("##DirShadowBias", &dirLight.shadowBias, 0.0f, 0.02f, "%.4f"))
					dirLight.shadowDirty = true;

				UI::PropertyLabel("Normal Bias");
				if (ImGui::SliderFloat("##DirNormalBias", &dirLight.normalBias, 0.0f, 0.05f, "%.4f"))
					dirLight.shadowDirty = true;

				UI::PropertyLabel("Blur Radius");
				ImGui::SliderFloat("##DirBlurRadius", &dirLight.blurRadius, 0.0f, 3.0f, "%.3f");

				UI::PropertyLabel("Reverse Cull Face");
				if (ImGui::Checkbox("##DirReverseCullFace", &dirLight.reverseCullFace))
					dirLight.shadowDirty = true;

				UI::PropertyLabel("Contact Shadows");
				ImGui::Checkbox("##DirContactShadows", &dirLight.contactShadows);
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
			if (ImGui::SliderFloat("##SpotRadius", &spotLight.radius, 0.1f, 100.0f, "%.2f"))
				spotLight.shadowDirty = true;

			UI::PropertyLabel("Inner Angle");
			if (ImGui::SliderFloat("##InnerAngle", &spotLight.innerAngle, 0.1f, 89.0f, "%.1f deg"))
			{
				if (spotLight.innerAngle > spotLight.outerAngle)
					spotLight.outerAngle = spotLight.innerAngle;
				spotLight.UpdateCutoffs();
				spotLight.shadowDirty = true;
			}

			UI::PropertyLabel("Outer Angle");
			if (ImGui::SliderFloat("##OuterAngle", &spotLight.outerAngle, 0.1f, 89.0f, "%.1f deg"))
			{
				if (spotLight.outerAngle < spotLight.innerAngle)
					spotLight.innerAngle = spotLight.outerAngle;
				spotLight.UpdateCutoffs();
				spotLight.shadowDirty = true;
			}

			UI::PropertyLabel("Cast Shadows");
			if (ImGui::Checkbox("##SpotCastShadows", &spotLight.castShadows))
				spotLight.shadowDirty = true;

			if (spotLight.castShadows)
			{
				UI::PropertyLabel("Static Shadow");
				if (ImGui::Checkbox("##SpotStaticShadow", &spotLight.staticShadow))
					spotLight.shadowDirty = true;

				if (spotLight.staticShadow)
				{
					UI::PropertyLabel("Update Shadow");
					if (ImGui::Button("Bake##SpotShadow"))
						spotLight.shadowDirty = true;
				}

				UI::PropertyLabel("Shadow Bias");
				if (ImGui::SliderFloat("##SpotShadowBias", &spotLight.shadowBias, 0.0f, 0.05f, "%.4f"))
					spotLight.shadowDirty = true;

				UI::PropertyLabel("Blur Radius");
				ImGui::SliderFloat("##SpotBlurRadius", &spotLight.blurRadius, 0.0f, 3.0f, "%.3f");

				UI::PropertyLabel("Reverse Cull Face");
				if (ImGui::Checkbox("##SpotReverseCullFace", &spotLight.reverseCullFace))
					spotLight.shadowDirty = true;
			}

			UI::EndPropertyTable();
		}
	});
}

} // namespace rv
