#include "MeshInspectors.h"
#include "InspectorUIUtils.h"
#include "InspectorContext.h"
#include "ImGui/imgui.h"
#include "EditorUtils.h"
#include "Scene/Entity.h"
#include "Scene/Components/VisibilityRangeComponent.h"
#include <filesystem>
#include <vector>
#include <string>

namespace rv {

void MeshInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<MeshComponent>("Mesh", registry, selectedEntity, [selectedEntity](MeshComponent& mesh)
	{
		static std::vector<SubAssetEntry> s_DroppedModelMeshSubs;
		static std::string s_DroppedModelName;
		static bool s_OpenDroppedModelPopup = false;

		std::string meshName = EditorUtils::GetAssetFileName(mesh.GetMeshID());
		std::string displayText = meshName.empty() ? "None (Mesh Slot)" : meshName;

		float btnHeight = 24.0f;
		float clearBtnW = mesh.GetMeshID().IsValid() ? 26.0f : 0.0f;
		float mainSlotW = ImGui::GetContentRegionAvail().x - (clearBtnW > 0 ? (clearBtnW + 4.0f) : 0.0f);

		if (ImGui::Button(displayText.c_str(), ImVec2(mainSlotW, btnHeight)))
		{
			ImGui::OpenPopup("SelectMeshPopup");
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
			{
				std::string pathStr((const char*)payload->Data);
				if (pathStr.ends_with(".obj") || pathStr.ends_with(".fbx") || pathStr.ends_with(".gltf") || pathStr.ends_with(".glb") || pathStr.ends_with(".rmesh") || pathStr.find('#') != std::string::npos)
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Assign Mesh", [&]() {
						AssetUUID directUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
						if (pathStr.find('#') != std::string::npos && directUUID.IsValid())
						{
							mesh.SetMesh(directUUID);
							EditorUtils::ClearAssetFileNameCache();
							return;
						}

						auto metaPath = AssetMeta::GetMetaPath(pathStr);
						AssetMeta meta;
						if (meta.LoadFromFile(metaPath))
						{
							std::vector<SubAssetEntry> meshSubs;
							for (const auto& sub : meta.subAssets)
							{
								if (sub.uuid.IsValid() && sub.type == "Mesh")
									meshSubs.push_back(sub);
							}
							if (!meshSubs.empty())
							{
								mesh.SetMesh(meshSubs[0].uuid);
								EditorUtils::ClearAssetFileNameCache();
								if (meshSubs.size() > 1)
								{
									s_DroppedModelMeshSubs = meshSubs;
									s_DroppedModelName = std::filesystem::path(pathStr).filename().string();
									s_OpenDroppedModelPopup = true;
								}
							}
							else
							{
								if (directUUID.IsValid()) mesh.SetMesh(directUUID);
							}
						}
						else
						{
							if (directUUID.IsValid()) mesh.SetMesh(directUUID);
						}
					});
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (clearBtnW > 0)
		{
			ImGui::SameLine(0, 4.0f);
			if (ImGui::Button("X##ClearMesh", ImVec2(clearBtnW, btnHeight)))
			{
				InspectorContext::ApplyEntityChange(selectedEntity, "Clear Mesh", [&]() {
					mesh.SetMesh(AssetUUID::Invalid());
					EditorUtils::ClearAssetFileNameCache();
				});
			}
		}

		if (s_OpenDroppedModelPopup)
		{
			ImGui::OpenPopup("SelectDroppedSubmeshPopup");
			s_OpenDroppedModelPopup = false;
		}

		if (ImGui::BeginPopup("SelectDroppedSubmeshPopup"))
		{
			ImGui::TextDisabled("Select Part from '%s'", s_DroppedModelName.c_str());
			ImGui::Separator();
			for (const auto& sub : s_DroppedModelMeshSubs)
			{
				bool isSelected = (mesh.GetMeshID() == sub.uuid);
				std::string partName = sub.name.empty() ? ("Mesh " + std::to_string(sub.index)) : sub.name;
				if (ImGui::Selectable(partName.c_str(), isSelected))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Select Submesh", [&]() {
						mesh.SetMesh(sub.uuid);
						EditorUtils::ClearAssetFileNameCache();
					});
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("SelectMeshPopup"))
		{
			static char filterText[128] = "";
			ImGui::TextDisabled("Select Mesh");
			ImGui::Separator();
			if (ImGui::IsWindowAppearing())
			{
				ImGui::SetKeyboardFocusHere();
				filterText[0] = '\0';
			}
			ImGui::SetNextItemWidth(300.0f);
			ImGui::InputTextWithHint("##MeshSearchInput", "Search mesh...", filterText, sizeof(filterText));
			ImGui::Separator();

			auto meshList = EditorUtils::GetAvailableMeshAssets(false);
			std::string filterStr = filterText;
			std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

			ImGui::BeginChild("MeshListScroll", ImVec2(300.0f, 220.0f), true);
			for (const auto& item : meshList)
			{
				std::string itemLower = item.fullName;
				std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), ::tolower);
				if (!filterStr.empty() && itemLower.find(filterStr) == std::string::npos)
					continue;

				bool isSelected = (mesh.GetMeshID() == item.uuid);
				if (ImGui::Selectable(item.fullName.c_str(), isSelected))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Select Mesh", [&]() {
						mesh.SetMesh(item.uuid);
						EditorUtils::ClearAssetFileNameCache();
					});
					ImGui::CloseCurrentPopup();
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndChild();

			if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, 22.0f)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	});

	UI::DrawComponent<SkeletalMeshComponent>("Skeletal Mesh", registry, selectedEntity, [selectedEntity](SkeletalMeshComponent& mesh)
	{
		static std::vector<SubAssetEntry> s_DroppedSkelModelMeshSubs;
		static std::string s_DroppedSkelModelName;
		static bool s_OpenDroppedSkelModelPopup = false;

		std::string skelMeshName = EditorUtils::GetAssetFileName(mesh.GetMeshID());
		std::string displayText = skelMeshName.empty() ? "None (Skeletal Mesh Slot)" : skelMeshName;

		float btnHeight = 24.0f;
		float clearBtnW = mesh.GetMeshID().IsValid() ? 26.0f : 0.0f;
		float mainSlotW = ImGui::GetContentRegionAvail().x - (clearBtnW > 0 ? (clearBtnW + 4.0f) : 0.0f);

		if (ImGui::Button(displayText.c_str(), ImVec2(mainSlotW, btnHeight)))
		{
			ImGui::OpenPopup("SelectSkelMeshPopup");
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
			{
				std::string pathStr((const char*)payload->Data);
				if (pathStr.ends_with(".obj") || pathStr.ends_with(".fbx") || pathStr.ends_with(".gltf") || pathStr.ends_with(".glb") || pathStr.ends_with(".rskmesh") || pathStr.find('#') != std::string::npos)
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Assign Skeletal Mesh", [&]() {
						AssetUUID directUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
						if (pathStr.find('#') != std::string::npos && directUUID.IsValid())
						{
							mesh.SetMesh(directUUID);
							EditorUtils::ClearAssetFileNameCache();
							return;
						}

						auto metaPath = AssetMeta::GetMetaPath(pathStr);
						AssetMeta meta;
						if (meta.LoadFromFile(metaPath))
						{
							std::vector<SubAssetEntry> meshSubs;
							for (const auto& sub : meta.subAssets)
							{
								if (sub.uuid.IsValid() && sub.type == "SkeletalMesh")
									meshSubs.push_back(sub);
							}
							if (!meshSubs.empty())
							{
								mesh.SetMesh(meshSubs[0].uuid);
								EditorUtils::ClearAssetFileNameCache();
								if (meshSubs.size() > 1)
								{
									s_DroppedSkelModelMeshSubs = meshSubs;
									s_DroppedSkelModelName = std::filesystem::path(pathStr).filename().string();
									s_OpenDroppedSkelModelPopup = true;
								}
							}
							else
							{
								if (directUUID.IsValid()) mesh.SetMesh(directUUID);
							}
						}
						else
						{
							if (directUUID.IsValid()) mesh.SetMesh(directUUID);
						}
					});
				}
			}
			ImGui::EndDragDropTarget();
		}

		if (clearBtnW > 0)
		{
			ImGui::SameLine(0, 4.0f);
			if (ImGui::Button("X##ClearSkelMesh", ImVec2(clearBtnW, btnHeight)))
			{
				InspectorContext::ApplyEntityChange(selectedEntity, "Clear Skeletal Mesh", [&]() {
					mesh.SetMesh(AssetUUID::Invalid());
					EditorUtils::ClearAssetFileNameCache();
				});
			}
		}

		if (s_OpenDroppedSkelModelPopup)
		{
			ImGui::OpenPopup("SelectDroppedSkelSubmeshPopup");
			s_OpenDroppedSkelModelPopup = false;
		}

		if (ImGui::BeginPopup("SelectDroppedSkelSubmeshPopup"))
		{
			ImGui::TextDisabled("Select Part from '%s'", s_DroppedSkelModelName.c_str());
			ImGui::Separator();
			for (const auto& sub : s_DroppedSkelModelMeshSubs)
			{
				bool isSelected = (mesh.GetMeshID() == sub.uuid);
				std::string partName = sub.name.empty() ? ("Mesh " + std::to_string(sub.index)) : sub.name;
				if (ImGui::Selectable(partName.c_str(), isSelected))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Select Skeletal Submesh", [&]() {
						mesh.SetMesh(sub.uuid);
						EditorUtils::ClearAssetFileNameCache();
					});
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("SelectSkelMeshPopup"))
		{
			static char filterText[128] = "";
			ImGui::TextDisabled("Select Skeletal Mesh");
			ImGui::Separator();
			if (ImGui::IsWindowAppearing())
			{
				ImGui::SetKeyboardFocusHere();
				filterText[0] = '\0';
			}
			ImGui::SetNextItemWidth(300.0f);
			ImGui::InputTextWithHint("##SkelSearchInput", "Search skeletal mesh...", filterText, sizeof(filterText));
			ImGui::Separator();

			auto meshList = EditorUtils::GetAvailableMeshAssets(true);
			std::string filterStr = filterText;
			std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

			ImGui::BeginChild("SkelMeshListScroll", ImVec2(300.0f, 220.0f), true);
			for (const auto& item : meshList)
			{
				std::string itemLower = item.fullName;
				std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), ::tolower);
				if (!filterStr.empty() && itemLower.find(filterStr) == std::string::npos)
					continue;

				bool isSelected = (mesh.GetMeshID() == item.uuid);
				if (ImGui::Selectable(item.fullName.c_str(), isSelected))
				{
					InspectorContext::ApplyEntityChange(selectedEntity, "Select Skeletal Mesh", [&]() {
						mesh.SetMesh(item.uuid);
						EditorUtils::ClearAssetFileNameCache();
					});
					ImGui::CloseCurrentPopup();
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndChild();

			if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, 22.0f)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	});

	UI::DrawComponent<SkeletonComponent>("Skeleton", registry, selectedEntity, [&](SkeletonComponent& skelComp)
	{
		std::string currentAssetName = EditorUtils::GetAssetFileName(skelComp.GetSkeletonID());
		UI::DrawFullWidthAssetDropSlot("Skeleton Asset Slot", currentAssetName, [&](const std::string& pathStr)
		{
			if (pathStr.ends_with(".rskeleton") || pathStr.ends_with(".fbx"))
			{
				AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
				if (uuid.IsValid()) skelComp.SetSkeleton(uuid);
				else LOG_ERROR("Could not find UUID for skeleton: {}", pathStr);
			}
		});

		auto skelAsset = skelComp.GetSkeleton();
		if (skelAsset && skelAsset->IsValid())
		{
			ImGui::Spacing();
			if (UI::BeginPropertyTable("SkeletonInfoTable"))
			{
				UI::PropertyLabel("Bone Count");
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%u bones", skelAsset->GetBoneCount());

				UI::PropertyLabel("Runtime State");
				if (skelComp.isInitialized)
					ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "Initialized (Active)");
				else
					ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.2f, 1.0f), "Not Initialized");

				UI::PropertyLabel("Skinning Palette");
				ImGui::Text("%zu / 100 matrices", skelComp.skinningPalette.size());

				UI::EndPropertyTable();
			}

			ImGui::Spacing();
			if (ImGui::Button("Reset to Bind Pose", ImVec2(-FLT_MIN, 24.0f)))
			{
				skelComp.InitFromAsset();
			}

			if (ImGui::CollapsingHeader("Bone Hierarchy"))
			{
				ImGui::BeginChild("BoneTreeRegion", ImVec2(-FLT_MIN, 180.0f), true);

				uint32_t count = skelAsset->GetBoneCount();
				for (uint32_t i = 0; i < count; ++i)
				{
					int32_t parent = skelAsset->GetParentIndex(i);
					std::string boneName = skelAsset->GetBoneName(i);

					ImGui::Bullet();
					ImGui::Text("[%02u] %s", i, boneName.c_str());
					ImGui::SameLine();
					if (parent == -1)
						ImGui::TextDisabled("(Root)");
					else
						ImGui::TextDisabled("-> Parent: [%02d]", parent);
				}

				ImGui::EndChild();
			}
		}
	});

	UI::DrawComponent<MeshRendererComponent>("Mesh Renderer", registry, selectedEntity, [](MeshRendererComponent& meshRenderer)
	{
		if (UI::BeginPropertyTable("MeshRendererTable"))
		{
			UI::PropertyLabel("Cast Shadows");
			bool isCastShadow = meshRenderer.IsCastShadow();
			if (ImGui::Checkbox("##CastShadow", &isCastShadow))
			{
				meshRenderer.SetCastShadow(isCastShadow);
				if (Scene* scene = InspectorContext::GetScene())
					scene->SetDirty(true);
			}

			UI::PropertyLabel("Enable Auto LOD");
			bool autoLOD = meshRenderer.IsAutoLODEnabled();
			if (ImGui::Checkbox("##EnableAutoLOD", &autoLOD))
			{
				meshRenderer.SetAutoLODEnabled(autoLOD);
				if (Scene* scene = InspectorContext::GetScene())
					scene->SetDirty(true);
			}

			if (meshRenderer.IsAutoLODEnabled())
			{
				UI::PropertyLabel("LOD Bias");
				float bias = meshRenderer.GetLODBias();
				if (ImGui::SliderFloat("##LODBias", &bias, 0.1f, 5.0f, "%.2f"))
				{
					meshRenderer.SetLODBias(bias);
					if (Scene* scene = InspectorContext::GetScene())
						scene->SetDirty(true);
				}
			}

			if (meshRenderer.gpuMesh && !meshRenderer.gpuMesh->lods.empty())
			{
				size_t lodCount = meshRenderer.gpuMesh->lods.size();
				UI::PropertyLabel("LOD Levels");
				ImGui::Text("%zu level%s", lodCount, lodCount > 1 ? "s" : "");

				UI::PropertyLabel("Active LOD");
				uint32_t cur = meshRenderer.GetCurrentLOD();
				const ImVec4 lodColors[] = {
					ImVec4(0.1f, 0.95f, 0.1f, 1.0f),
					ImVec4(0.1f, 0.75f, 1.0f, 1.0f),
					ImVec4(1.0f, 0.85f, 0.1f, 1.0f),
					ImVec4(1.0f, 0.2f,  0.2f, 1.0f)
				};
				ImVec4 color = (cur < 4) ? lodColors[cur] : lodColors[3];
				uint32_t triCount = (cur < lodCount) ? (meshRenderer.gpuMesh->lods[cur].indexCount / 3) : (meshRenderer.indexCount / 3);
				ImGui::TextColored(color, "LOD %u  (%u tris)", cur, triCount);
			}

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<SkeletalMeshRendererComponent>("Skeletal Mesh Renderer", registry, selectedEntity, [](SkeletalMeshRendererComponent& skeletalMeshRenderer)
	{
		if (UI::BeginPropertyTable("SkeletalMeshRendererTable"))
		{
			UI::PropertyLabel("Cast Shadows");
			bool isCastShadow = skeletalMeshRenderer.IsCastShadow();
			if (ImGui::Checkbox("##CastShadow", &isCastShadow))
			{
				skeletalMeshRenderer.SetCastShadow(isCastShadow);
				if (Scene* scene = InspectorContext::GetScene())
					scene->SetDirty(true);
			}

			UI::EndPropertyTable();
		}
	});

	UI::DrawComponent<VisibilityRangeComponent>("Visibility Range", registry, selectedEntity, [](VisibilityRangeComponent& visRange)
	{
		if (UI::BeginPropertyTable("VisibilityRangeTable"))
		{
			bool changed = false;

			UI::PropertyLabel("Begin");
			if (ImGui::DragFloat("##VisBegin", &visRange.begin, 0.1f, 0.0f, 10000.0f, "%.2f m"))
			{
				if (visRange.begin < 0.0f) visRange.begin = 0.0f;
				changed = true;
			}

			UI::PropertyLabel("Begin Margin");
			if (ImGui::DragFloat("##VisBeginMargin", &visRange.beginMargin, 0.1f, 0.0f, 1000.0f, "%.2f m"))
			{
				if (visRange.beginMargin < 0.0f) visRange.beginMargin = 0.0f;
				changed = true;
			}

			UI::PropertyLabel("End");
			if (ImGui::DragFloat("##VisEnd", &visRange.end, 0.1f, 0.0f, 10000.0f, "%.2f m"))
			{
				if (visRange.end < 0.0f) visRange.end = 0.0f;
				changed = true;
			}

			UI::PropertyLabel("End Margin");
			if (ImGui::DragFloat("##VisEndMargin", &visRange.endMargin, 0.1f, 0.0f, 1000.0f, "%.2f m"))
			{
				if (visRange.endMargin < 0.0f) visRange.endMargin = 0.0f;
				changed = true;
			}

			UI::PropertyLabel("Fade");
			if (ImGui::Checkbox("##VisFade", &visRange.fade))
			{
				changed = true;
			}

			if (changed)
			{
				if (Scene* scene = InspectorContext::GetScene())
					scene->SetDirty(true);
			}

			UI::EndPropertyTable();
		}
	});
}

} // namespace rv
