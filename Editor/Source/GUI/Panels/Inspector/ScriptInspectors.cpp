#include "ScriptInspectors.h"
#include "InspectorUIUtils.h"
#include "InspectorContext.h"
#include "ImGui/imgui.h"
#include "EditorUtils.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"
#include "Script/ScriptEngine.h"
#include "Script/ScriptProperty.h"
#include "Scene/Entity.h"
#include <cstring>
#include <string>
#include <algorithm>

namespace rv {

void ScriptInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<ScriptComponent>("Script", registry, selectedEntity, [&](ScriptComponent& scriptComp)
	{
		std::string scriptName = EditorUtils::GetAssetFileName(scriptComp.scriptAssetUUID);
		UI::DrawFullWidthAssetDropSlot("Script Slot", scriptName, [&](const std::string& pathStr)
		{
			if (pathStr.ends_with(".lua"))
			{
				auto& assetRegistry = AssetManager::Get().GetRegistry();
				AssetMeta scriptMeta = assetRegistry.GetOrCreateMeta(pathStr);
				AssetUUID scriptUUID = scriptMeta.uuid;

				if (scriptUUID != scriptComp.scriptAssetUUID)
				{
					scriptComp.scriptAssetUUID = scriptUUID;
					scriptComp.propertyValues = json::object();
					ScriptEngine::InvalidateScriptPropertyDefs(scriptUUID);
				}
			}
		});

		if (!scriptComp.scriptAssetUUID.IsValid())
			return;

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		float availWidth = ImGui::GetContentRegionAvail().x;
		ImGui::TextDisabled("Script Properties");
		ImGui::SameLine(availWidth - 55.0f);
		const bool refreshProperties = ImGui::SmallButton("Refresh");
		if (refreshProperties)
		{
			ScriptEngine::InvalidateScriptPropertyDefs(scriptComp.scriptAssetUUID);
		}

		ImGui::Spacing();
		const auto& propDefs = ScriptEngine::GetScriptPropertyDefs(scriptComp.scriptAssetUUID);
		if (propDefs.empty())
		{
			ImGui::TextDisabled("No exported properties.");
			return;
		}

		if (UI::BeginPropertyTable("ScriptPropertiesTable", 0.40f))
		{
			for (const auto& prop : propDefs)
			{
				switch (prop.type)
				{
				case ScriptPropertyType::Float:
				{
					UI::PropertyLabel(prop.name.c_str());
					float val = prop.floatVal;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_number())
						val = scriptComp.propertyValues[prop.name].get<float>();

					std::string id = "##" + prop.name;
					bool changed = false;
					if (prop.hasRange)
						changed = ImGui::SliderFloat(id.c_str(), &val, prop.minVal, prop.maxVal, "%.2f");
					else
						changed = ImGui::DragFloat(id.c_str(), &val, prop.step, 0.0f, 0.0f, "%.2f");

					if (changed)
					{
						scriptComp.propertyValues[prop.name] = val;
						if (scriptComp.luaInstance.valid())
							scriptComp.luaInstance[prop.name] = val;
					}
					break;
				}
				case ScriptPropertyType::Int:
				{
					UI::PropertyLabel(prop.name.c_str());
					int val = prop.intVal;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_number())
						val = scriptComp.propertyValues[prop.name].get<int>();

					std::string id = "##" + prop.name;
					bool changed = false;
					if (prop.hasRange)
						changed = ImGui::SliderInt(id.c_str(), &val, static_cast<int>(prop.minVal), static_cast<int>(prop.maxVal));
					else
						changed = ImGui::DragInt(id.c_str(), &val, std::max(1.0f, prop.step));

					if (changed)
					{
						scriptComp.propertyValues[prop.name] = val;
						if (scriptComp.luaInstance.valid())
							scriptComp.luaInstance[prop.name] = val;
					}
					break;
				}
				case ScriptPropertyType::Bool:
				{
					UI::PropertyLabel(prop.name.c_str());
					bool val = prop.boolVal;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_boolean())
						val = scriptComp.propertyValues[prop.name].get<bool>();

					std::string id = "##" + prop.name;
					if (ImGui::Checkbox(id.c_str(), &val))
					{
						scriptComp.propertyValues[prop.name] = val;
						if (scriptComp.luaInstance.valid())
							scriptComp.luaInstance[prop.name] = val;
					}
					break;
				}
				case ScriptPropertyType::String:
				{
					UI::PropertyLabel(prop.name.c_str());
					std::string val = prop.stringVal;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_string())
						val = scriptComp.propertyValues[prop.name].get<std::string>();

					char buf[512];
					strncpy(buf, val.c_str(), sizeof(buf));
					buf[sizeof(buf) - 1] = '\0';
					std::string id = "##" + prop.name;
					if (ImGui::InputText(id.c_str(), buf, sizeof(buf)))
					{
						scriptComp.propertyValues[prop.name] = std::string(buf);
						if (scriptComp.luaInstance.valid())
							scriptComp.luaInstance[prop.name] = std::string(buf);
					}
					break;
				}
				case ScriptPropertyType::Vec2:
				{
					glm::vec2 val = prop.vec2Val;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_array() && scriptComp.propertyValues[prop.name].size() >= 2)
					{
						val.x = scriptComp.propertyValues[prop.name][0].get<float>();
						val.y = scriptComp.propertyValues[prop.name][1].get<float>();
					}

					glm::vec2 prevVal = val;
					UI::DrawVec2Control(prop.name.c_str(), val, 0.0f, prop.step);
					if (val != prevVal)
					{
						scriptComp.propertyValues[prop.name] = { val.x, val.y };
						if (scriptComp.luaInstance.valid())
							scriptComp.luaInstance[prop.name] = val;
					}
					break;
				}
				case ScriptPropertyType::Vec3:
				{
					glm::vec3 val = prop.vec3Val;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_array() && scriptComp.propertyValues[prop.name].size() >= 3)
					{
						val.x = scriptComp.propertyValues[prop.name][0].get<float>();
						val.y = scriptComp.propertyValues[prop.name][1].get<float>();
						val.z = scriptComp.propertyValues[prop.name][2].get<float>();
					}

					glm::vec3 prevVal = val;
					UI::DrawVec3Control(prop.name.c_str(), val, 0.0f, prop.step);
					if (val != prevVal)
					{
						scriptComp.propertyValues[prop.name] = { val.x, val.y, val.z };
						if (scriptComp.luaInstance.valid())
							scriptComp.luaInstance[prop.name] = val;
					}
					break;
				}
				case ScriptPropertyType::Vec4:
				{
					UI::PropertyLabel(prop.name.c_str());
					glm::vec4 val = prop.vec4Val;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_array() && scriptComp.propertyValues[prop.name].size() >= 4)
					{
						val.x = scriptComp.propertyValues[prop.name][0].get<float>();
						val.y = scriptComp.propertyValues[prop.name][1].get<float>();
						val.z = scriptComp.propertyValues[prop.name][2].get<float>();
						val.w = scriptComp.propertyValues[prop.name][3].get<float>();
					}

					std::string id = "##" + prop.name;
					if (ImGui::DragFloat4(id.c_str(), &val.x, prop.step, 0.0f, 0.0f, "%.2f"))
					{
						scriptComp.propertyValues[prop.name] = { val.x, val.y, val.z, val.w };
						if (scriptComp.luaInstance.valid())
							scriptComp.luaInstance[prop.name] = val;
					}
					break;
				}
				case ScriptPropertyType::Color:
				{
					UI::PropertyLabel(prop.name.c_str());
					glm::vec4 val = prop.vec4Val;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_array() && scriptComp.propertyValues[prop.name].size() >= 4)
					{
						val.r = scriptComp.propertyValues[prop.name][0].get<float>();
						val.g = scriptComp.propertyValues[prop.name][1].get<float>();
						val.b = scriptComp.propertyValues[prop.name][2].get<float>();
						val.a = scriptComp.propertyValues[prop.name][3].get<float>();
					}

					std::string id = "##" + prop.name;
					if (ImGui::ColorEdit4(id.c_str(), &val.r))
					{
						scriptComp.propertyValues[prop.name] = { val.r, val.g, val.b, val.a };
						if (scriptComp.luaInstance.valid())
							scriptComp.luaInstance[prop.name] = val;
					}
					break;
				}
				case ScriptPropertyType::AssetHandle:
				{
					UI::PropertyLabel(prop.name.c_str());
					AssetUUID val = prop.assetVal;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_string())
					{
						val = AssetUUID::FromString(scriptComp.propertyValues[prop.name].get<std::string>());
					}

					std::string assetName = val.IsValid() ? EditorUtils::GetAssetFileName(val) : "<Empty>";
					std::string btnId = assetName + "##" + prop.name;
					float totalAvail = ImGui::GetContentRegionAvail().x;
					float clearBtnW = 22.0f;
					float slotW = std::max(totalAvail - clearBtnW - 4.0f, 20.0f);

					ImGui::Button(btnId.c_str(), ImVec2(slotW, 22.0f));
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
						{
							std::string pathStr((const char*)payload->Data);
							AssetUUID droppedUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
							if (!droppedUUID.IsValid())
								droppedUUID = AssetManager::Get().GetRegistry().GetUUID(pathStr);

							if (droppedUUID.IsValid())
							{
								InspectorContext::ApplyEntityChange(selectedEntity, "Set Script Property: " + prop.name, [&]() {
									scriptComp.propertyValues[prop.name] = droppedUUID.ToString();
									if (scriptComp.luaInstance.valid())
										scriptComp.luaInstance[prop.name] = droppedUUID;
								});
							}
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SameLine(0, 4.0f);
					std::string clearId = "X##" + prop.name;
					if (ImGui::Button(clearId.c_str(), ImVec2(clearBtnW, 22.0f)))
					{
						InspectorContext::ApplyEntityChange(selectedEntity, "Clear Script Property: " + prop.name, [&]() {
							scriptComp.propertyValues[prop.name] = AssetUUID{}.ToString();
							if (scriptComp.luaInstance.valid())
								scriptComp.luaInstance[prop.name] = AssetUUID{};
						});
					}
					break;
				}
				case ScriptPropertyType::Entity:
				{
					UI::PropertyLabel(prop.name.c_str());
					EntityHandle value = prop.entityVal;
					if (scriptComp.propertyValues.contains(prop.name) && scriptComp.propertyValues[prop.name].is_string())
					{
						try { value.uuid = std::stoull(scriptComp.propertyValues[prop.name].get<std::string>()); }
						catch (const std::exception&) { value = {}; }
					}
					const EntityUUID originalUUID = value.uuid;

					std::string entityName = "<None>";
					if (value.IsValid())
					{
						const auto uuidIt = scene.GetUUIDEntityMap().find(value.uuid);
						if (uuidIt != scene.GetUUIDEntityMap().end() && registry.valid(uuidIt->second) && registry.any_of<TagComponent>(uuidIt->second))
							entityName = registry.get<TagComponent>(uuidIt->second).tag;
						else
							entityName = "<Missing Entity>";
					}

					const float totalAvail = ImGui::GetContentRegionAvail().x;
					constexpr float clearButtonWidth = 22.0f;
					const float slotWidth = std::max(totalAvail - clearButtonWidth - 4.0f, 20.0f);
					const std::string slotId = entityName + "##EntityProperty_" + prop.name;
					ImGui::Button(slotId.c_str(), ImVec2(slotWidth, 22.0f));
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG"))
						{
							if (payload->DataSize == sizeof(entt::entity))
							{
								const entt::entity draggedEntity = *static_cast<const entt::entity*>(payload->Data);
								if (registry.valid(draggedEntity) && registry.any_of<UUIDComponent>(draggedEntity))
									value.uuid = registry.get<UUIDComponent>(draggedEntity).uuid;
							}
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SameLine(0, 4.0f);
					const std::string clearId = "X##EntityProperty_" + prop.name;
					if (ImGui::Button(clearId.c_str(), ImVec2(clearButtonWidth, 22.0f)))
						value = {};

					if (value.uuid != originalUUID)
					{
						InspectorContext::ApplyEntityChange(selectedEntity, "Set Entity Property: " + prop.name, [&]() {
							scriptComp.propertyValues[prop.name] = value.IsValid() ? std::to_string(value.uuid) : std::string{};
							Entity resolved = value.Resolve(scene);
							if (scriptComp.luaInstance.valid())
								scriptComp.luaInstance[prop.name] = resolved;
						});
					}
					break;
				}
				}
			}
			UI::EndPropertyTable();
		}
	});
}

} // namespace rv
