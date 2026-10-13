#include "InspectorHeader.h"
#include "ImGui/imgui.h"
#include "EditorUtils.h"
#include "EditorSelection.h"
#include "AssetImporters/PrefabImporter.h"
#include "Scene/SceneSerializer.h"
#include "Scene/Entity.h"
#include "Commands/CommandHistory.h"
#include "Commands/PrefabCommands.h"
#include <cstring>
#include <string>

namespace rv {

bool InspectorHeader::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	if (registry.any_of<TagComponent>(selectedEntity))
	{
		auto& tag = registry.get<TagComponent>(selectedEntity);
		char buffer[256];
		std::strncpy(buffer, tag.tag.c_str(), sizeof(buffer));
		buffer[sizeof(buffer) - 1] = '\0';

		ImGui::Checkbox("##EntityEnabled", &tag.isActive);
		ImGui::SameLine();

		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::InputText("##TagName", buffer, sizeof(buffer)))
		{
			if (buffer[0] != '\0')
				tag.tag = buffer;
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	entt::entity prefabRoot = entt::null;
	if (registry.any_of<PrefabComponent>(selectedEntity))
	{
		prefabRoot = selectedEntity;
	}
	else
	{
		entt::entity curr = scene.GetParent(selectedEntity);
		while (curr != entt::null && curr != scene.GetRootEntity())
		{
			if (registry.any_of<PrefabComponent>(curr))
			{
				prefabRoot = curr;
				break;
			}
			curr = scene.GetParent(curr);
		}
	}

	if (prefabRoot != entt::null && prefabRoot == selectedEntity)
	{
		auto& prefabComp = registry.get<PrefabComponent>(selectedEntity);
		AssetUUID pUUID = prefabComp.GetPrefabID();
		std::string prefabName = EditorUtils::GetAssetFileName(pUUID);
		if (prefabName.empty()) prefabName = "Prefab Asset";

		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.35f, 0.55f, 0.8f));
		if (ImGui::CollapsingHeader("Prefab Instance", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextDisabled("Source:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", prefabName.c_str());

			static entt::entity s_CachedPrefabEntity = entt::null;
			static json s_CachedOverrides = json::array();
			static double s_LastOverrideCheckTime = 0.0;
			static bool s_ForceOverridesRefresh = false;

			double currentTime = ImGui::GetTime();
			if (selectedEntity != s_CachedPrefabEntity || s_ForceOverridesRefresh || (currentTime - s_LastOverrideCheckTime) > 0.5)
			{
				s_CachedPrefabEntity = selectedEntity;
				s_CachedOverrides = SceneSerializer::ComputePrefabOverrides(scene, selectedEntity);
				s_LastOverrideCheckTime = currentTime;
				s_ForceOverridesRefresh = false;
			}

			const json& overrides = s_CachedOverrides;
			bool hasOverrides = overrides.is_array() && !overrides.empty();

			ImGui::TextDisabled("Overrides:");
			ImGui::SameLine();
			if (hasOverrides)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "%zu modified properties", overrides.size());
				if (ImGui::TreeNode("View Overrides"))
				{
					for (const auto& op : overrides)
					{
						std::string path = op.value("path", "");
						std::string opType = op.value("op", "");
						ImGui::BulletText("[%s] %s", opType.c_str(), path.c_str());
					}
					ImGui::TreePop();
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(0.5f, 0.85f, 0.5f, 1.0f), "In Sync");
			}

			float btnWidth = (ImGui::GetContentRegionAvail().x - 16.0f) / 3.0f;
			if (ImGui::Button("Apply", ImVec2(btnWidth, 24.0f)))
			{
				auto cmd = std::make_unique<ApplyPrefabCommand>(&scene, selectedEntity);
				PrefabImporter::ApplyPrefab(scene, selectedEntity);
				CommandHistory::Get().Execute(std::move(cmd));
				s_ForceOverridesRefresh = true;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Save current hierarchy and component changes back to the .rprefab file");

			ImGui::SameLine();
			if (ImGui::Button("Revert", ImVec2(btnWidth, 24.0f)))
			{
				auto cmd = std::make_unique<RevertPrefabCommand>(&scene, selectedEntity);
				PrefabImporter::RevertPrefab(scene, selectedEntity);
				cmd->CaptureAfter(EditorSelection::Get().GetPrimary());
				CommandHistory::Get().Execute(std::move(cmd));
				s_ForceOverridesRefresh = true;
				// selectedEntity might have been replaced
				ImGui::PopStyleColor();
				return false;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Discard local changes and reload from the .rprefab file");

			ImGui::SameLine();
			if (ImGui::Button("Unpack", ImVec2(btnWidth, 24.0f)))
			{
				auto cmd = std::make_unique<UnpackPrefabCommand>(&scene, selectedEntity);
				scene.RemoveComponent<PrefabComponent>(selectedEntity);
				CommandHistory::Get().Execute(std::move(cmd));
				s_CachedPrefabEntity = entt::null;
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Convert this instance into standard scene entities (Break Prefab link)");
		}
		ImGui::PopStyleColor();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}
	else if (prefabRoot != entt::null)
	{
		auto& prefabComp = registry.get<PrefabComponent>(prefabRoot);
		AssetUUID pUUID = prefabComp.GetPrefabID();
		std::string prefabName = EditorUtils::GetAssetFileName(pUUID);
		if (prefabName.empty()) prefabName = "Prefab Asset";

		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.22f, 0.35f, 0.55f, 0.8f));
		if (ImGui::CollapsingHeader("Prefab (Child Entity)", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextDisabled("Source:");
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.62f, 0.56f, 0.80f, 1.0f), "%s", prefabName.c_str());

			float btnWidth = (ImGui::GetContentRegionAvail().x - 12.0f) / 2.0f;
			if (ImGui::Button("Select Root", ImVec2(btnWidth, 24.0f)))
			{
				EditorSelection::Get().Select(prefabRoot);
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Select the root entity of this prefab instance");

			ImGui::SameLine();
			if (ImGui::Button("Apply Prefab", ImVec2(btnWidth, 24.0f)))
			{
				auto cmd = std::make_unique<ApplyPrefabCommand>(&scene, prefabRoot);
				PrefabImporter::ApplyPrefab(scene, prefabRoot);
				CommandHistory::Get().Execute(std::move(cmd));
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Save changes back to the .rprefab file");
		}
		ImGui::PopStyleColor();
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
	}

	return true;
}

} // namespace rv
