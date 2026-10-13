#include "InspectorPanel.h"
#include "Inspector/InspectorContext.h"
#include "Inspector/InspectorHeader.h"
#include "Inspector/TransformInspectors.h"
#include "Inspector/MeshInspectors.h"
#include "Inspector/LightInspectors.h"
#include "Inspector/PhysicsInspectors.h"
#include "Inspector/MaterialInspectors.h"
#include "Inspector/ScriptInspectors.h"
#include "Inspector/MediaInspectors.h"
#include "Inspector/UIInspectors.h"
#include "Inspector/AddComponentPopup.h"

#include "Core/Engine.h"
#include "EditorSelection.h"
#include "Scene/SceneSerializer.h"
#include "Commands/CommandHistory.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

namespace rv {

void InspectorPanel::Draw(Engine* engine)
{
	entt::entity selectedEntity = EditorSelection::Get().GetPrimary();

	Scene& scene = engine->GetActiveScene();
	InspectorContext::SetScene(&scene);
	InspectorContext::SetCurrentEntity(selectedEntity);
	entt::registry& registry = scene.GetRegistry();

	ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

	if (selectedEntity == entt::null || !registry.valid(selectedEntity))
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (size.y * 0.4f));
		const char* msg = "Select an entity to inspect";
		float textWidth = ImGui::CalcTextSize(msg).x;
		ImGui::SetCursorPosX((size.x - textWidth) * 0.5f);
		ImGui::TextDisabled("%s", msg);
		ImGui::End();
		return;
	}

	if (!InspectorHeader::Draw(scene, selectedEntity))
	{
		ImGui::End();
		return;
	}

	TransformInspectors::DrawTransform(scene, selectedEntity);
	MeshInspectors::Draw(scene, selectedEntity);
	LightInspectors::Draw(scene, selectedEntity);
	PhysicsInspectors::Draw(scene, selectedEntity);
	MaterialInspectors::Draw(scene, selectedEntity);
	TransformInspectors::DrawHierarchy(scene, selectedEntity);
	ScriptInspectors::Draw(scene, selectedEntity);
	MediaInspectors::Draw(scene, selectedEntity);
	UIInspectors::Draw(scene, selectedEntity);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	AddComponentPopup::Draw(scene, selectedEntity);

	if ((ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)) && GImGui && GImGui->ActiveIdHasBeenEditedThisFrame)
	{
		scene.SetDirty(true);
	}

	static entt::entity s_TrackingEntity = entt::null;
	static ImGuiID s_LastActiveId = 0;
	static json s_BeforeSnapshot;

	ImGuiWindow* inspectorWindow = ImGui::GetCurrentWindow();
	ImGuiID activeId = GImGui ? GImGui->ActiveId : 0;
	ImGuiWindow* activeWindow = GImGui ? GImGui->ActiveIdWindow : nullptr;

	bool activeInInspector = (activeWindow && inspectorWindow && (activeWindow == inspectorWindow || activeWindow->RootWindow == inspectorWindow->RootWindow));

	if (s_LastActiveId != 0 && (activeId != s_LastActiveId || !activeInInspector))
	{
		if (s_TrackingEntity != entt::null && registry.valid(s_TrackingEntity))
		{
			json afterSnapshot = SceneSerializer::SerializeEntity(scene, s_TrackingEntity, false);
			if (!s_BeforeSnapshot.is_null() && !s_BeforeSnapshot.empty() && s_BeforeSnapshot != afterSnapshot)
			{
				auto cmd = std::make_unique<EntitySnapshotCommand>(&scene, s_TrackingEntity, s_BeforeSnapshot, afterSnapshot, "Edit Property");
				CommandHistory::Get().Execute(std::move(cmd));
			}
		}
		s_LastActiveId = 0;
		s_TrackingEntity = entt::null;
		s_BeforeSnapshot.clear();
	}

	if (activeId != 0 && activeInInspector && s_LastActiveId == 0)
	{
		s_TrackingEntity = selectedEntity;
		s_LastActiveId = activeId;
		if (s_TrackingEntity != entt::null && registry.valid(s_TrackingEntity))
		{
			s_BeforeSnapshot = SceneSerializer::SerializeEntity(scene, s_TrackingEntity, false);
		}
	}

	ImGui::End();
}

} // namespace rv
