#pragma once
#include "InspectorContext.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Render/IconLibrary.h"
#include "Commands/CommandHistory.h"
#include "Commands/ComponentCommands.h"
#include "Scene/Entity.h"
#include <glm/glm.hpp>
#include <string>
#include <algorithm>
#include <type_traits>

namespace rv::UI {

bool BeginPropertyTable(const char* id = "PropertyTable", float labelRatio = 0.40f);
void EndPropertyTable();
void PropertyLabel(const char* label);
bool DrawTransformRow(const char* label, glm::vec3& values, float resetValue = 0.0f, float speed = 0.1f, float min = 0.0f, float max = 0.0f, bool isScale = false, bool* lockRatio = nullptr);
void DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float speed = 0.1f, float min = 0.0f, float max = 0.0f);
void DrawVec2Control(const char* label, glm::vec2& values, float resetValue = 0.0f, float speed = 0.1f, float min = 0.0f, float max = 0.0f);

template<typename DropFunc>
void DrawFullWidthAssetDropSlot(const char* hintText, const std::string& currentPath, DropFunc&& onDropAsset)
{
	std::string displayText = currentPath.empty() ? hintText : currentPath;
	ImGui::Button(displayText.c_str(), ImVec2(-FLT_MIN, 24.0f));

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
		{
			std::string pathStr((const char*)payload->Data);
			InspectorContext::ApplyEntityChange(InspectorContext::GetCurrentEntity(), std::string("Assign Asset: ") + hintText, [&]() {
				onDropAsset(pathStr);
			});
		}
		ImGui::EndDragDropTarget();
	}
}

template<typename T, typename UIFunction>
void DrawComponent(const std::string& name, entt::registry& registry, entt::entity entity, UIFunction uiFunction, bool removable = true)
{
	if (!registry.any_of<T>(entity))
		return;

	auto& component = registry.get<T>(entity);
	bool isRemoved = false;

	ImGui::PushID((void*)typeid(T).hash_code());

	ImVec2 contentRegion = ImGui::GetContentRegionAvail();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));

	ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed |
		ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
		ImGuiTreeNodeFlags_FramePadding;

	bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());

	if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		ImGui::OpenPopup("ComponentSettings");

	float lineHeight = ImGui::GetFrameHeight();
	float buttonSize = lineHeight - 4.0f;

	if constexpr (std::is_same_v<T, TransformComponent>)
	{
		ImGui::SameLine(contentRegion.x - buttonSize * 2.0f - 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.40f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.68f, 0.62f, 0.88f, 0.60f));
		if (ImGui::Button("R##resetAllTransform", ImVec2(buttonSize, buttonSize)))
		{
			component.Reset();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Reset all Transform properties (Position, Rotation, Scale)");
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine(contentRegion.x - buttonSize);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.40f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.68f, 0.62f, 0.88f, 0.60f));
		if (ImGui::Button("...", ImVec2(buttonSize, buttonSize)))
			ImGui::OpenPopup("ComponentSettings");
		ImGui::PopStyleColor(3);

		if (ImGui::BeginPopup("ComponentSettings"))
		{
			if (ImGui::MenuItem("Reset Transform"))
			{
				component.Reset();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Reset Position"))
			{
				component.ResetPosition();
			}
			if (ImGui::MenuItem("Reset Rotation"))
			{
				component.ResetRotation();
			}
			if (ImGui::MenuItem("Reset Scale"))
			{
				component.ResetScale();
			}
			ImGui::Separator();
			auto& clipboard = InspectorContext::GetTransformClipboard();
			if (ImGui::MenuItem("Copy Transform"))
			{
				clipboard.position = component.GetPosition();
				clipboard.rotation = component.GetEulerRotation();
				clipboard.scale = component.GetScale();
				clipboard.hasTransform = true;
			}
			if (ImGui::MenuItem("Paste Transform", nullptr, false, clipboard.hasTransform))
			{
				component.SetPosition(clipboard.position);
				component.SetEulerRotation(clipboard.rotation);
				component.SetScale(clipboard.scale);
				component.SetDirty();
			}
			ImGui::EndPopup();
		}
	}
	else if (removable)
	{
		ImGui::SameLine(contentRegion.x - buttonSize);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.40f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.68f, 0.62f, 0.88f, 0.60f));
		if (ImGui::Button("...", ImVec2(buttonSize, buttonSize)))
			ImGui::OpenPopup("ComponentSettings");
		ImGui::PopStyleColor(3);

		if (ImGui::BeginPopup("ComponentSettings"))
		{
			if (ImGui::MenuItem("Remove Component"))
				isRemoved = true;
			ImGui::EndPopup();
		}
	}

	ImGui::PopStyleVar();

	if (open)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 3.0f));
		uiFunction(component);
		ImGui::PopStyleVar();
		ImGui::TreePop();
		ImGui::Spacing();
	}

	if (isRemoved)
	{
		Scene* scene = InspectorContext::GetScene();
		if (scene)
		{
			auto cmd = std::make_unique<ComponentCommand<T>>(scene, entity, ComponentCommand<T>::Action::Remove, name);
			CommandHistory::Get().Execute(std::move(cmd));
		}
		registry.remove<T>(entity);
	}

	ImGui::PopID();
}

template<typename T, typename... Args>
void DrawAddComponentEntry(const char* label, const char* searchFilter, entt::registry& registry, entt::entity entity, Args&&... args)
{
	if (registry.any_of<T>(entity))
		return;

	if (searchFilter && searchFilter[0] != '\0')
	{
		std::string filterStr = searchFilter;
		std::string labelStr = label;
		auto toLower = [](std::string& s) {
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		};
		toLower(filterStr);
		toLower(labelStr);
		if (labelStr.find(filterStr) == std::string::npos)
			return;
	}

	if (ImGui::MenuItem(label))
	{
		registry.emplace<T>(entity, std::forward<Args>(args)...);
		if constexpr (std::is_same_v<T, UICanvasComponent> ||
					  std::is_same_v<T, UIImageComponent> ||
					  std::is_same_v<T, UITextComponent> ||
					  std::is_same_v<T, UIButtonComponent> ||
					  std::is_same_v<T, UISliderComponent> ||
					  std::is_same_v<T, UIProgressBarComponent> ||
					  std::is_same_v<T, UICheckboxComponent> ||
					  std::is_same_v<T, UIPanelComponent> ||
					  std::is_same_v<T, UILayoutGroupComponent> ||
					  std::is_same_v<T, UITextInputComponent> ||
					  std::is_same_v<T, UIScrollRectComponent> ||
					  std::is_same_v<T, UITabContainerComponent>)
		{
			if (!registry.any_of<RectTransformComponent>(entity))
			{
				registry.emplace<RectTransformComponent>(entity);
			}
		}
		Scene* scene = InspectorContext::GetScene();
		if (scene)
		{
			auto cmd = std::make_unique<ComponentCommand<T>>(scene, entity, ComponentCommand<T>::Action::Add, label);
			CommandHistory::Get().Execute(std::move(cmd));
		}
		ImGui::CloseCurrentPopup();
	}
}

} // namespace rv::UI
