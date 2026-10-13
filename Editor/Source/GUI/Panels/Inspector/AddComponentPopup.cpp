#include "AddComponentPopup.h"
#include "InspectorUIUtils.h"
#include "ImGui/imgui.h"
#include "Scene/Entity.h"

namespace rv {

void AddComponentPopup::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	float availWidth = ImGui::GetContentRegionAvail().x;
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - 180.0f) * 0.5f);
	if (ImGui::Button("Add Component", ImVec2(180.0f, 0)))
		ImGui::OpenPopup("AddComponentPopup");

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		static char filterBuf[128] = "";
		ImGui::SetNextItemWidth(200.0f);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputTextWithHint("##Filter", "Search Components...", filterBuf, sizeof(filterBuf));
		ImGui::Separator();

		UI::DrawAddComponentEntry<ScriptComponent>("Script", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<MaterialComponent>("Material", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<MeshComponent>("Mesh", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<MeshRendererComponent>("Mesh Renderer", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<SkeletalMeshComponent>("Skeletal Mesh", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<SkeletalMeshRendererComponent>("Skeletal Mesh Renderer", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<PointLightComponent>("Point Light", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<DirectionalLightComponent>("Directional Light", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<SpotLightComponent>("Spot Light", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<RigidbodyComponent>("Rigidbody", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<CharacterBodyComponent>("CharacterBody", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<BoxColliderComponent>("Box Collider", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<SphereColliderComponent>("Sphere Collider", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<CapsuleColliderComponent>("Capsule Collider", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<CylinderColliderComponent>("Cylinder Collider", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<MeshColliderComponent>("Mesh Collider", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<ConvexHullColliderComponent>("Convex Hull Collider", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<AudioEmitterComponent>("Audio Emitter", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<ParticleEmitterComponent>("Particle Emitter", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<DecalComponent>("Decal", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<AnimatorComponent>("Animator", filterBuf, registry, selectedEntity);

		UI::DrawAddComponentEntry<UICanvasComponent>("UI Canvas", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<RectTransformComponent>("Rect Transform", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<UIImageComponent>("UI Image", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<UITextComponent>("UI Text", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<UIButtonComponent>("UI Button", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<UISliderComponent>("UI Slider", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<UIProgressBarComponent>("UI Progress Bar", filterBuf, registry, selectedEntity);
		UI::DrawAddComponentEntry<UICheckboxComponent>("UI Checkbox", filterBuf, registry, selectedEntity);

		if (!ImGui::IsPopupOpen("AddComponentPopup"))
			filterBuf[0] = '\0';

		ImGui::EndPopup();
	}
}

} // namespace rv
