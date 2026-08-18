#include "InspectorPanel.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Core/Engine.h"
#include "Renderer/RenderLayer.h"
#include "EditorUtils.h"
#include "AssetImporters/MaterialSerializer.h"
#include "Audio/AudioManager.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <string>

using namespace rv;

namespace UI
{
	static bool BeginPropertyTable(const char* id = "PropertyTable")
	{
		return ImGui::BeginTable(id, 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoBordersInBody);
	}

	static void EndPropertyTable()
	{
		ImGui::EndTable();
	}

	static void PropertyLabel(const char* label)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, 24.0f);
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(label);
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
	}

	static void DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float speed = 0.1f, float min = 0.0f, float max = 0.0f)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, 24.0f);
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.35f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.68f, 0.62f, 0.88f, 0.50f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
		if (ImGui::Button("R", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
		{
			values = glm::vec3(resetValue);
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);

		ImGui::SameLine(0, 4.0f);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);

		ImGuiStyle& style = ImGui::GetStyle();
		float buttonWidth = ImGui::GetFrameHeight();
		float totalWidth = ImGui::GetContentRegionAvail().x;
		float itemSpacing = style.ItemSpacing.x;
		float inputWidth = std::max((totalWidth - (buttonWidth * 3.0f) - (itemSpacing * 5.0f)) / 3.0f, 1.0f);

		auto DrawAxis = [&](const char* axisLabel, float& val, const ImVec4& colNormal, const ImVec4& colHover, const ImVec4& colActive)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, colNormal);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colHover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, colActive);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
				if (ImGui::Button(axisLabel, ImVec2(buttonWidth, buttonWidth)))
					val = resetValue;
				ImGui::PopStyleVar();
				ImGui::PopStyleColor(3);

				ImGui::SameLine(0, 2.0f);
				ImGui::SetNextItemWidth(inputWidth);
				std::string dragLabel = std::string("##") + axisLabel;
				ImGui::DragFloat(dragLabel.c_str(), &val, speed, min, max, "%.2f");
			};

		DrawAxis("X", values.x, ImVec4(0.75f, 0.20f, 0.20f, 1.0f), ImVec4(0.85f, 0.30f, 0.30f, 1.0f), ImVec4(0.65f, 0.15f, 0.15f, 1.0f));
		ImGui::SameLine(0, itemSpacing);
		DrawAxis("Y", values.y, ImVec4(0.20f, 0.70f, 0.20f, 1.0f), ImVec4(0.30f, 0.80f, 0.30f, 1.0f), ImVec4(0.15f, 0.60f, 0.15f, 1.0f));
		ImGui::SameLine(0, itemSpacing);
		DrawAxis("Z", values.z, ImVec4(0.20f, 0.45f, 0.85f, 1.0f), ImVec4(0.30f, 0.55f, 0.95f, 1.0f), ImVec4(0.15f, 0.35f, 0.75f, 1.0f));

		ImGui::PopID();
	}

	static void DrawFullWidthAssetDropSlot(const char* hintText, const std::string& currentPath, auto onDropAsset)
	{
		std::string displayText = currentPath.empty() ? hintText : currentPath;
		ImGui::Button(displayText.c_str(), ImVec2(-FLT_MIN, 24.0f));

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
			{
				std::string pathStr((const char*)payload->Data);
				onDropAsset(pathStr);
			}
			ImGui::EndDragDropTarget();
		}
	}
}

template<typename T, typename UIFunction>
static void DrawComponent(const std::string& name, entt::registry& registry, entt::entity entity, UIFunction uiFunction, bool removable = true)
{
	if (!registry.any_of<T>(entity))
		return;

	auto& component = registry.get<T>(entity);
	bool isRemoved = false;

	ImGui::PushID((void*)typeid(T).hash_code());

	ImVec2 contentRegion = ImGui::GetContentRegionAvail();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 6.0f));

	ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
		ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
		ImGuiTreeNodeFlags_FramePadding;

	bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());

	float lineHeight = ImGui::GetFrameHeight();
	float buttonSize = lineHeight - 4.0f;

	if (removable)
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
		registry.remove<T>(entity);

	ImGui::PopID();
}

template<typename T, typename... Args>
static void DrawAddComponentEntry(const char* label, const char* searchFilter, entt::registry& registry, entt::entity entity, Args&&... args)
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
		ImGui::CloseCurrentPopup();
	}
}

void InspectorPanel::Draw(Engine* engine, entt::entity& selectedEntity)
{
	Scene& scene = engine->GetActiveScene();
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

	if (registry.any_of<TagComponent>(selectedEntity))
	{
		auto& tag = registry.get<TagComponent>(selectedEntity);
		char buffer[256];
		std::strncpy(buffer, tag.tag.c_str(), sizeof(buffer));
		buffer[sizeof(buffer) - 1] = '\0';

		static bool enabled = true;
		ImGui::Checkbox("##EntityEnabled", &enabled);
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

	bool isPrefab = registry.any_of<PrefabComponent>(selectedEntity);

	DrawComponent<TransformComponent>("Transform", registry, selectedEntity, [](TransformComponent& transform)
		{
			if (UI::BeginPropertyTable("TransformTable"))
			{
				glm::vec3 pos = transform.GetPosition();
				glm::vec3 euler = transform.GetEulerRotation();
				glm::vec3 scale = transform.GetScale();
				bool isScaleRatioLocked = transform.IsScaleRatioLocked();

				glm::vec3 oldPos = pos;
				glm::vec3 oldEuler = euler;
				glm::vec3 oldScale = scale;

				UI::DrawVec3Control("Position", pos, 0.0f, 0.1f);
				UI::DrawVec3Control("Rotation", euler, 0.0f, 0.1f, -360.0f, 360.0f);
				UI::DrawVec3Control("Scale", scale, 1.0f, 0.05f, 0.001f, 10000.0f);

				if (pos != oldPos) { transform.SetPosition(pos); transform.SetDirty(); }
				if (euler != oldEuler) { transform.SetEulerRotation(euler); transform.SetDirty(); }
				if (scale != oldScale) { transform.SetScale(scale); transform.SetDirty(); }

				UI::PropertyLabel("Lock Ratio");
				if (ImGui::Checkbox("##LockRatio", &isScaleRatioLocked))
					transform.SetLockScaleRatio(isScaleRatioLocked);

				UI::EndPropertyTable();
			}
		}, false);

	if (!isPrefab)
	{
		DrawComponent<MeshComponent>("Mesh", registry, selectedEntity, [](MeshComponent& mesh)
			{
				UI::DrawFullWidthAssetDropSlot("Mesh Slot", "", [&](const std::string& pathStr)
					{
						if (pathStr.ends_with(".obj") || pathStr.ends_with(".fbx") || pathStr.ends_with(".gltf") || pathStr.ends_with(".glb") || pathStr.ends_with(".rmesh"))
						{
							AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
							if (uuid.IsValid()) mesh.SetMesh(uuid);
							else LOG_ERROR("Could not find UUID for mesh: {}", pathStr);
						}
					});
			});

		DrawComponent<CameraComponent>("Camera", registry, selectedEntity, [](CameraComponent& camComp)
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

		DrawComponent<MeshRendererComponent>("Mesh Renderer", registry, selectedEntity, [](MeshRendererComponent& meshRenderer)
			{
				if (UI::BeginPropertyTable("MeshRendererTable"))
				{
					UI::PropertyLabel("Cast Shadows");
					bool isCastShadow = meshRenderer.IsCastShadow();
					if (ImGui::Checkbox("##CastShadow", &isCastShadow))
						meshRenderer.SetCastShadow(isCastShadow);

					UI::EndPropertyTable();
				}
			});

		DrawComponent<PointLightComponent>("Point Light", registry, selectedEntity, [](PointLightComponent& pointLight)
			{
				if (UI::BeginPropertyTable("PointLightTable"))
				{
					UI::PropertyLabel("Color");
					float color[3] = { pointLight.color.r, pointLight.color.g, pointLight.color.b };
					if (ImGui::ColorEdit3("##LightColor", color, ImGuiColorEditFlags_NoInputs))
						pointLight.color = glm::vec3(color[0], color[1], color[2]);

					UI::PropertyLabel("Intensity");
					ImGui::SliderFloat("##Intensity", &pointLight.intensity, 0.0f, 20.0f, "%.2f");

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

		DrawComponent<DirectionalLightComponent>("Directional Light", registry, selectedEntity, [](DirectionalLightComponent& dirLight)
			{
				if (UI::BeginPropertyTable("DirLightTable"))
				{
					UI::PropertyLabel("Color");
					float color[3] = { dirLight.color.r, dirLight.color.g, dirLight.color.b };
					if (ImGui::ColorEdit3("##DirLightColor", color, ImGuiColorEditFlags_NoInputs))
						dirLight.color = glm::vec3(color[0], color[1], color[2]);

					UI::PropertyLabel("Intensity");
					ImGui::SliderFloat("##DirIntensity", &dirLight.intensity, 0.0f, 10.0f, "%.2f");

					UI::PropertyLabel("Cast Shadows");
					ImGui::Checkbox("##DirCastShadows", &dirLight.castShadows);

					if (dirLight.castShadows)
					{
						UI::PropertyLabel("Shadow Bias");
						ImGui::SliderFloat("##DirShadowBias", &dirLight.shadowBias, 0.0f, 0.1f, "%.4f");

						UI::PropertyLabel("Blur Radius");
						ImGui::SliderFloat("##DirBlurRadius", &dirLight.blurRadius, 0.0f, 2.0f, "%.3f");

						UI::PropertyLabel("Reverse Cull Face");
						ImGui::Checkbox("##DirReverseCullFace", &dirLight.reverseCullFace);
					}

					UI::EndPropertyTable();
				}
			});

		DrawComponent<RigidbodyComponent>("Rigidbody", registry, selectedEntity, [](RigidbodyComponent& rb)
			{
				if (UI::BeginPropertyTable("RigidbodyTable"))
				{
					UI::PropertyLabel("Body Type");
					const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
					int currentType = static_cast<int>(rb.bodyType);
					if (ImGui::Combo("##BodyType", &currentType, bodyTypes, 3))
						rb.bodyType = static_cast<Physics::MotionType>(currentType);

					UI::PropertyLabel("Auto Mass");
					ImGui::Checkbox("##AutoMass", &rb.autoCalculateMass);

					if (!rb.autoCalculateMass)
					{
						UI::PropertyLabel("Mass");
						ImGui::SliderFloat("##Mass", &rb.mass, 0.001f, 1000.0f, "%.3f kg");
					}

					UI::PropertyLabel("Friction");
					ImGui::SliderFloat("##Friction", &rb.friction, 0.0f, 1.0f, "%.2f");

					UI::PropertyLabel("Restitution");
					ImGui::SliderFloat("##Restitution", &rb.restitution, 0.0f, 1.0f, "%.2f");

					UI::PropertyLabel("Linear Damping");
					ImGui::SliderFloat("##LinearDamping", &rb.linearDamping, 0.0f, 1.0f, "%.2f");

					UI::PropertyLabel("Angular Damping");
					ImGui::SliderFloat("##AngularDamping", &rb.angularDamping, 0.0f, 1.0f, "%.2f");

					UI::PropertyLabel("Max Linear Vel");
					ImGui::SliderFloat("##MaxLinearVel", &rb.maxLinearVelocity, 0.0f, 1000.0f, "%.1f");

					UI::PropertyLabel("Max Angular Vel");
					ImGui::SliderFloat("##MaxAngularVel", &rb.maxAngularVelocity, 0.0f, 100.0f, "%.1f");

					UI::PropertyLabel("Gravity Factor");
					ImGui::SliderFloat("##GravityFactor", &rb.gravityFactor, 0.0f, 10.0f, "%.2f");

					UI::PropertyLabel("Is Sensor");
					ImGui::Checkbox("##IsSensor", &rb.isSensor);

					UI::PropertyLabel("Use CCD");
					ImGui::Checkbox("##UseCCD", &rb.useCCD);

					UI::PropertyLabel("Allow Sleeping");
					ImGui::Checkbox("##AllowSleep", &rb.allowSleep);

					UI::EndPropertyTable();
				}

				ImGui::Spacing();
				if (ImGui::CollapsingHeader("Constraints"))
				{
					if (UI::BeginPropertyTable("ConstraintsTable"))
					{
						UI::PropertyLabel("Freeze Position");
						ImGui::Checkbox("X##FrzPosX", &rb.lockTranslationX); ImGui::SameLine();
						ImGui::Checkbox("Y##FrzPosY", &rb.lockTranslationY); ImGui::SameLine();
						ImGui::Checkbox("Z##FrzPosZ", &rb.lockTranslationZ);

						UI::PropertyLabel("Freeze Rotation");
						ImGui::Checkbox("X##FrzRotX", &rb.lockRotationX); ImGui::SameLine();
						ImGui::Checkbox("Y##FrzRotY", &rb.lockRotationY); ImGui::SameLine();
						ImGui::Checkbox("Z##FrzRotZ", &rb.lockRotationZ);

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Collision Filter"))
				{
					auto DrawBitGrid = [](const char* headerTitle, uint32_t& bitField)
						{
							ImGui::TextDisabled("%s", headerTitle);
							float btnSize = (ImGui::GetContentRegionAvail().x - (7.0f * 4.0f)) / 8.0f;
							btnSize = std::max(btnSize, 18.0f);

							for (int i = 0; i < 32; i++)
							{
								bool active = (bitField & (1u << i)) != 0;
								if (active)
									ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.56f, 0.80f, 0.85f));
								else
									ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.17f, 0.17f, 0.23f, 0.60f));

								ImGui::PushID(i);
								if (ImGui::Button(std::to_string(i + 1).c_str(), ImVec2(btnSize, btnSize)))
								{
									if (active) bitField &= ~(1u << i);
									else bitField |= (1u << i);
								}
								ImGui::PopID();
								ImGui::PopStyleColor();

								if ((i % 8) != 7)
									ImGui::SameLine(0, 4.0f);
							}
						};

					DrawBitGrid("Layer Mask", rb.collisionFilter.layer);
					ImGui::Spacing();
					DrawBitGrid("Collision Mask", rb.collisionFilter.mask);
				}
			});

		DrawComponent<BoxColliderComponent>("Box Collider", registry, selectedEntity, [](BoxColliderComponent& bc)
			{
				if (UI::BeginPropertyTable("BoxColliderTable"))
				{
					glm::vec3 size = bc.GetSize();
					glm::vec3 offset = bc.GetOffset();
					UI::DrawVec3Control("Size", size, 1.0f, 0.05f, 0.001f, 1000.0f);
					UI::DrawVec3Control("Offset", offset, 0.0f, 0.05f);
					bc.SetSize(size);
					bc.SetOffset(offset);
					UI::EndPropertyTable();
				}
			});

		DrawComponent<SphereColliderComponent>("Sphere Collider", registry, selectedEntity, [](SphereColliderComponent& sc)
			{
				if (UI::BeginPropertyTable("SphereColliderTable"))
				{
					float radius = sc.GetRadius();
					glm::vec3 offset = sc.GetOffset();
					UI::PropertyLabel("Radius");
					if (ImGui::DragFloat("##SphereRadius", &radius, 0.05f, 0.001f, 1000.0f, "%.2f")) sc.SetRadius(radius);
					UI::DrawVec3Control("Offset", offset, 0.0f, 0.05f);
					sc.SetOffset(offset);
					UI::EndPropertyTable();
				}
			});

		DrawComponent<CapsuleColliderComponent>("Capsule Collider", registry, selectedEntity, [](CapsuleColliderComponent& cc)
			{
				if (UI::BeginPropertyTable("CapsuleColliderTable"))
				{
					float halfHeight = cc.GetHalfHeight();
					float radius = cc.GetRadius();
					glm::vec3 offset = cc.GetOffset();
					UI::PropertyLabel("Half Height");
					if (ImGui::DragFloat("##CapsuleHalfHeight", &halfHeight, 0.05f, 0.001f, 100.0f, "%.2f")) cc.SetHalfHeight(halfHeight);
					UI::PropertyLabel("Radius");
					if (ImGui::DragFloat("##CapsuleRadius", &radius, 0.05f, 0.001f, 100.0f, "%.2f")) cc.SetRadius(radius);
					UI::DrawVec3Control("Offset", offset, 0.0f, 0.05f);
					cc.SetOffset(offset);
					UI::EndPropertyTable();
				}
			});

		DrawComponent<CylinderColliderComponent>("Cylinder Collider", registry, selectedEntity, [](CylinderColliderComponent& cc)
			{
				if (UI::BeginPropertyTable("CylinderColliderTable"))
				{
					float halfHeight = cc.GetHalfHeight();
					float radius = cc.GetRadius();
					glm::vec3 offset = cc.GetOffset();
					UI::PropertyLabel("Half Height");
					if (ImGui::DragFloat("##CylinderHalfHeight", &halfHeight, 0.05f, 0.001f, 100.0f, "%.2f")) cc.SetHalfHeight(halfHeight);
					UI::PropertyLabel("Radius");
					if (ImGui::DragFloat("##CylinderRadius", &radius, 0.05f, 0.001f, 100.0f, "%.2f")) cc.SetRadius(radius);
					UI::DrawVec3Control("Offset", offset, 0.0f, 0.05f);
					cc.SetOffset(offset);
					UI::EndPropertyTable();
				}
			});

		DrawComponent<MeshColliderComponent>("Mesh Collider", registry, selectedEntity, [](MeshColliderComponent& mc)
			{
				if (UI::BeginPropertyTable("MeshColliderTable"))
				{
					int maxTris = mc.GetMaxTrianglesPerLeaf();
					float edgeAngle = mc.GetActiveEdgeTresholdAngle();
					glm::vec3 offset = mc.GetOffset();
					UI::PropertyLabel("Max Tris / Leaf");
					if (ImGui::SliderInt("##MaxTris", &maxTris, 1, 16)) mc.SetMaxTrianglesPerLeaf(maxTris);
					UI::PropertyLabel("Edge Threshold");
					if (ImGui::SliderFloat("##EdgeAngle", &edgeAngle, 0.001f, 2.0f, "%.3f")) mc.SetActiveEdgeTresholdAngle(edgeAngle);
					UI::DrawVec3Control("Offset", offset, 0.0f, 0.05f);
					mc.SetOffset(offset);
					UI::EndPropertyTable();
				}
			});

		DrawComponent<ConvexHullColliderComponent>("Convex Hull Collider", registry, selectedEntity, [](ConvexHullColliderComponent& chc)
			{
				if (UI::BeginPropertyTable("ConvexHullTable"))
				{
					float maxConvexRadius = chc.GetMaxConvexRadius();
					glm::vec3 offset = chc.GetOffset();
					UI::PropertyLabel("Max Convex Radius");
					if (ImGui::SliderFloat("##MaxRadius", &maxConvexRadius, 0.0f, 0.1f, "%.4f")) chc.SetMaxConvexRadius(maxConvexRadius);
					UI::DrawVec3Control("Offset", offset, 0.0f, 0.05f);
					chc.SetOffset(offset);
					UI::EndPropertyTable();
				}
			});

		DrawComponent<MaterialComponent>("Material", registry, selectedEntity, [](MaterialComponent& material)
			{
				const MaterialInstance& inst = material.GetInstance();

				float saveBtnWidth = inst.GetSourceAsset() ? 65.0f : 0.0f;
				float slotWidth = inst.GetSourceAsset() ? ImGui::GetContentRegionAvail().x - saveBtnWidth : -FLT_MIN;

				ImGui::Button("Material Slot", ImVec2(slotWidth, 24.0f));
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
					{
						std::string pathStr((const char*)payload->Data);
						if (pathStr.ends_with(".rmat"))
						{
							AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
							if (uuid.IsValid()) material.SetMaterial(uuid);
							else LOG_ERROR("Could not find UUID for material: {}", pathStr);
						}
					}
					ImGui::EndDragDropTarget();
				}

				if (inst.GetSourceAsset())
				{
					ImGui::SameLine();
					if (ImGui::Button("Save##matsave", ImVec2(saveBtnWidth - 6.0f, 24.0f)))
					{
						auto asset = inst.GetSourceAsset();
						if (inst.IsOverridden(MatField::AlbedoColor)) asset->albedoColor = material.GetAlbedoColor();
						if (inst.IsOverridden(MatField::EmissiveColor)) asset->emissiveColor = material.GetEmissiveColor();
						if (inst.IsOverridden(MatField::EmissiveIntensity)) asset->emissiveIntensity = material.GetEmissiveIntensity();
						if (inst.IsOverridden(MatField::Metallic)) asset->metallic = material.GetMetallic();
						if (inst.IsOverridden(MatField::Specular)) asset->specular = material.GetSpecular();
						if (inst.IsOverridden(MatField::Roughness)) asset->roughness = material.GetRoughness();
						if (inst.IsOverridden(MatField::AO)) asset->ao = material.GetAO();
						if (inst.IsOverridden(MatField::NormalScale)) asset->normalScale = material.GetNormalScale();
						if (inst.IsOverridden(MatField::HeightScale)) asset->heightScale = material.GetHeightScale();
						if (inst.IsOverridden(MatField::UVScale)) asset->UVScale = material.GetUVScale();
						if (inst.IsOverridden(MatField::UVOffset)) asset->UVOffset = material.GetUVOffset();
						if (inst.IsOverridden(MatField::TransparencyMode)) asset->transparencyMode = material.GetTransparencyMode();
						if (inst.IsOverridden(MatField::BlendMode)) asset->blendMode = material.GetBlendMode();
						if (inst.IsOverridden(MatField::CullMode)) asset->cullMode = material.GetCullMode();
						if (inst.IsOverridden(MatField::AlphaCutoff)) asset->alphaCutoff = material.GetAlphaCutoff();

						if (inst.IsOverridden(MatField::AlbedoTex)) { asset->albedoTextureUUID = material.GetAlbedoTexture() ? material.GetAlbedoTexture()->GetUUID() : AssetUUID{}; asset->useAlbedoMap = material.IsUsingAlbedoMap(); }
						if (inst.IsOverridden(MatField::NormalTex)) { asset->normalTextureUUID = material.GetNormalTexture() ? material.GetNormalTexture()->GetUUID() : AssetUUID{}; asset->useNormalMap = material.IsUsingNormalMap(); }
						if (inst.IsOverridden(MatField::MetallicTex)) { asset->metallicTextureUUID = material.GetMetallicTexture() ? material.GetMetallicTexture()->GetUUID() : AssetUUID{}; asset->useMetallicMap = material.IsUsingMetallicMap(); }
						if (inst.IsOverridden(MatField::RoughnessTex)) { asset->roughnessTextureUUID = material.GetRoughnessTexture() ? material.GetRoughnessTexture()->GetUUID() : AssetUUID{}; asset->useRoughnessMap = material.IsUsingRoughnessMap(); }
						if (inst.IsOverridden(MatField::AOTex)) { asset->aoTextureUUID = material.GetAOTexture() ? material.GetAOTexture()->GetUUID() : AssetUUID{}; asset->useAOMap = material.IsUsingAOMap(); }
						if (inst.IsOverridden(MatField::HeightTex)) { asset->heightTextureUUID = material.GetHeightTexture() ? material.GetHeightTexture()->GetUUID() : AssetUUID{}; asset->useHeightMap = material.IsUsingHeightMap(); }

						auto path = AssetManager::Get().GetRegistry().GetPath(asset->GetUUID());
						MaterialSerializer::Save(asset, path);
						material.GetInstance().ClearAllOverrides();
						material.GetInstance().RebuildCache();
					}
				}

				ImGui::Spacing();

				auto RevertButton = [](const char* id, auto clearFn) {
					ImGui::SameLine();
					if (ImGui::Button(id, ImVec2(22, 22))) clearFn();
					};

				auto TextureSlot = [&](const char* label, const char* idSuffix, bool overridden, auto setTexFn, auto clearTexFn) {
					UI::PropertyLabel(label);
					float btnWidth = overridden ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
					ImGui::Button("Texture", ImVec2(btnWidth, 22));
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
						{
							std::string pathStr((const char*)payload->Data);
							static constexpr std::array exts = { ".png", ".jpg", ".jpeg", ".tga", ".hdr", ".rtex" };
							bool isImage = std::any_of(exts.begin(), exts.end(), [&](const char* e) { return pathStr.ends_with(e); });
							if (isImage)
							{
								AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
								if (uuid.IsValid()) setTexFn(uuid);
								else LOG_ERROR("Could not find UUID for texture: {}", pathStr);
							}
						}
						ImGui::EndDragDropTarget();
					}
					if (overridden) {
						RevertButton((std::string("R##") + idSuffix).c_str(), clearTexFn);
					}
					};

				if (ImGui::CollapsingHeader("Transparency##transphdr"))
				{
					if (UI::BeginPropertyTable("TransparencyTable"))
					{
						UI::PropertyLabel("Mode");
						const char* transModes[] = { "Opaque", "Alpha", "Alpha Scissor", "Depth Prepass" };
						int currentTransMode = static_cast<int>(material.GetTransparencyMode());
						float itemW = inst.IsOverridden(MatField::TransparencyMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(itemW);
						if (ImGui::Combo("##tmode", &currentTransMode, transModes, 4))
							material.SetTransparencyMode(static_cast<TransparencyMode>(currentTransMode));
						if (inst.IsOverridden(MatField::TransparencyMode)) RevertButton("R##tmode", [&]() { material.ClearTransparencyMode(); });

						if (material.GetTransparencyMode() == TransparencyMode::AlphaScissor)
						{
							UI::PropertyLabel("Alpha Cutoff");
							float cutoff = material.GetAlphaCutoff();
							float cutW = inst.IsOverridden(MatField::AlphaCutoff) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
							ImGui::SetNextItemWidth(cutW);
							if (ImGui::SliderFloat("##acutoff", &cutoff, 0.0f, 1.0f, "%.2f")) material.SetAlphaCutoff(cutoff);
							if (inst.IsOverridden(MatField::AlphaCutoff)) RevertButton("R##acutoff", [&]() { material.ClearAlphaCutoff(); });
						}

						if (material.GetTransparencyMode() == TransparencyMode::Alpha || material.GetTransparencyMode() == TransparencyMode::DepthPrepass)
						{
							UI::PropertyLabel("Blend Mode");
							const char* blendModes[] = { "Mix", "Add", "Subtract", "Multiply" };
							int currentBlendMode = static_cast<int>(material.GetBlendMode());
							float blendW = inst.IsOverridden(MatField::BlendMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
							ImGui::SetNextItemWidth(blendW);
							if (ImGui::Combo("##bmode", &currentBlendMode, blendModes, 4))
								material.SetBlendMode(static_cast<BlendMode>(currentBlendMode));
							if (inst.IsOverridden(MatField::BlendMode)) RevertButton("R##bmode", [&]() { material.ClearBlendMode(); });
						}

						UI::PropertyLabel("Cull Mode");
						const char* cullModes[] = { "Back", "Front", "Disabled" };
						int currentCullMode = static_cast<int>(material.GetCullMode());
						float cullW = inst.IsOverridden(MatField::CullMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(cullW);
						if (ImGui::Combo("##cmode", &currentCullMode, cullModes, 3))
							material.SetCullMode(static_cast<CullMode>(currentCullMode));
						if (inst.IsOverridden(MatField::CullMode)) RevertButton("R##cmode", [&]() { material.ClearCullMode(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Shading##shadinghdr"))
				{
					if (UI::BeginPropertyTable("ShadingTable"))
					{
						UI::PropertyLabel("Shading Mode");
						const char* shadingModes[] = { "Lit", "Unshaded" };
						int currentShadingMode = static_cast<int>(material.GetShadingMode());
						float shdW = inst.IsOverridden(MatField::ShadingMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(shdW);
						if (ImGui::Combo("##smode", &currentShadingMode, shadingModes, 2))
							material.SetShadingMode(static_cast<ShadingMode>(currentShadingMode));
						if (inst.IsOverridden(MatField::ShadingMode)) RevertButton("R##smode", [&]() { material.ClearShadingMode(); });

						if (material.GetShadingMode() == ShadingMode::Lit)
						{
							UI::PropertyLabel("Receive Shadows");
							bool receiveShadows = material.GetReceiveShadows();
							if (ImGui::Checkbox("##rshadows", &receiveShadows)) material.SetReceiveShadows(receiveShadows);
							if (inst.IsOverridden(MatField::ReceiveShadows)) RevertButton("R##rshadows", [&]() { material.ClearReceiveShadows(); });
						}

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Albedo"))
				{
					if (UI::BeginPropertyTable("AlbedoTable"))
					{
						TextureSlot("Texture", "alb", inst.IsOverridden(MatField::AlbedoTex), [&](AssetUUID u) { material.SetAlbedoTexture(u); }, [&]() { material.ClearAlbedoTexture(); });

						UI::PropertyLabel("Color");
						glm::vec4 c = material.GetAlbedoColor();
						float col[4] = { c.r, c.g, c.b, c.a };
						float colW = inst.IsOverridden(MatField::AlbedoColor) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(colW);
						if (ImGui::ColorEdit4("##albedocol", col)) material.SetAlbedoColor({ col[0], col[1], col[2], col[3] });
						if (inst.IsOverridden(MatField::AlbedoColor)) RevertButton("R##albcol", [&]() { material.ClearAlbedoColor(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Normal"))
				{
					if (UI::BeginPropertyTable("NormalTable"))
					{
						TextureSlot("Texture", "nrm", inst.IsOverridden(MatField::NormalTex), [&](AssetUUID u) { material.SetNormalTexture(u); }, [&]() { material.ClearNormalTexture(); });

						UI::PropertyLabel("Scale");
						float normalScale = material.GetNormalScale();
						float scaleW = inst.IsOverridden(MatField::NormalScale) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(scaleW);
						if (ImGui::SliderFloat("##nrmscl", &normalScale, -10.0f, 10.0f, "%.2f")) material.SetNormalScale(normalScale);
						if (inst.IsOverridden(MatField::NormalScale)) RevertButton("R##nrmscl", [&]() { material.ClearNormalScale(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Roughness##rhdr"))
				{
					if (UI::BeginPropertyTable("RoughnessTable"))
					{
						TextureSlot("Texture", "rgh", inst.IsOverridden(MatField::RoughnessTex), [&](AssetUUID u) { material.SetRoughnessTexture(u); }, [&]() { material.ClearRoughnessTexture(); });

						UI::PropertyLabel("Roughness");
						float roughness = material.GetRoughness();
						float rghW = inst.IsOverridden(MatField::Roughness) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(rghW);
						if (ImGui::SliderFloat("##rghval", &roughness, 0.0f, 1.0f, "%.2f")) material.SetRoughness(roughness);
						if (inst.IsOverridden(MatField::Roughness)) RevertButton("R##rghval", [&]() { material.ClearRoughness(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Metallic##mhdr"))
				{
					if (UI::BeginPropertyTable("MetallicTable"))
					{
						TextureSlot("Texture", "mtl", inst.IsOverridden(MatField::MetallicTex), [&](AssetUUID u) { material.SetMetallicTexture(u); }, [&]() { material.ClearMetallicTexture(); });

						UI::PropertyLabel("Metallic");
						float metallicVal = material.GetMetallic();
						float mtlW = inst.IsOverridden(MatField::Metallic) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(mtlW);
						if (ImGui::SliderFloat("##mtlval", &metallicVal, 0.0f, 1.0f, "%.2f")) material.SetMetallic(metallicVal);
						if (inst.IsOverridden(MatField::Metallic)) RevertButton("R##mtlval", [&]() { material.ClearMetallic(); });

						UI::PropertyLabel("Specular");
						float specular = material.GetSpecular();
						float spcW = inst.IsOverridden(MatField::Specular) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(spcW);
						if (ImGui::SliderFloat("##spcval", &specular, 0.0f, 1.0f, "%.2f")) material.SetSpecular(specular);
						if (inst.IsOverridden(MatField::Specular)) RevertButton("R##spcval", [&]() { material.ClearSpecular(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Height"))
				{
					if (UI::BeginPropertyTable("HeightTable"))
					{
						TextureSlot("Texture", "hgt", inst.IsOverridden(MatField::HeightTex), [&](AssetUUID u) { material.SetHeightTexture(u); }, [&]() { material.ClearHeightTexture(); });

						UI::PropertyLabel("Scale");
						float heightScale = material.GetHeightScale();
						float hgtW = inst.IsOverridden(MatField::HeightScale) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(hgtW);
						if (ImGui::SliderFloat("##hgtscl", &heightScale, 0.0f, 1.0f, "%.3f")) material.SetHeightScale(heightScale);
						if (inst.IsOverridden(MatField::HeightScale)) RevertButton("R##hgtscl", [&]() { material.ClearHeightScale(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("AO##aohdr"))
				{
					if (UI::BeginPropertyTable("AOTable"))
					{
						TextureSlot("Texture", "ao", inst.IsOverridden(MatField::AOTex), [&](AssetUUID u) { material.SetAOTexture(u); }, [&]() { material.ClearAOTexture(); });

						UI::PropertyLabel("AO");
						float ao = material.GetAO();
						float aoW = inst.IsOverridden(MatField::AO) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(aoW);
						if (ImGui::SliderFloat("##aoval", &ao, 0.0f, 1.0f, "%.2f")) material.SetAO(ao);
						if (inst.IsOverridden(MatField::AO)) RevertButton("R##aoval", [&]() { material.ClearAO(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Emissive"))
				{
					if (UI::BeginPropertyTable("EmissiveTable"))
					{
						UI::PropertyLabel("Color");
						glm::vec3 ec = material.GetEmissiveColor();
						float col[3] = { ec.r, ec.g, ec.b };
						float colW = inst.IsOverridden(MatField::EmissiveColor) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(colW);
						if (ImGui::ColorEdit3("##emcol", col)) material.SetEmissiveColor({ col[0], col[1], col[2] });
						if (inst.IsOverridden(MatField::EmissiveColor)) RevertButton("R##emcol", [&]() { material.ClearEmissiveColor(); });

						UI::PropertyLabel("Intensity");
						float intensity = material.GetEmissiveIntensity();
						float intW = inst.IsOverridden(MatField::EmissiveIntensity) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(intW);
						if (ImGui::SliderFloat("##emint", &intensity, 0.0f, 20.0f, "%.1f")) material.SetEmissiveIntensity(intensity);
						if (inst.IsOverridden(MatField::EmissiveIntensity)) RevertButton("R##emint", [&]() { material.ClearEmissiveIntensity(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Sampler"))
				{
					if (UI::BeginPropertyTable("SamplerTable"))
					{
						const char* minFilterTypes[] = { "Nearest", "Linear", "Nearest Mipmap", "Linear Mipmap" };
						const char* magFilterTypes[] = { "Nearest", "Linear" };
						const char* wrapTypes[] = { "Repeat", "Mirror Repeat", "Clamp to edge", "Clamp to border", "Mirror clamp to edge" };
						const char* anisoLevels[] = { "1x", "2x", "4x", "8x", "16x" };
						SamplerDesc desc = material.GetSampler().GetDesc();
						int minF = static_cast<uint8_t>(desc.minFilter);
						int magF = static_cast<uint8_t>(desc.magFilter);
						int wrap = static_cast<uint8_t>(desc.wrap);
						int aniso = desc.anisotropy > 8.0f ? 4 : desc.anisotropy > 4.0f ? 3 : desc.anisotropy > 2.0f ? 2 : desc.anisotropy > 1.0f ? 1 : 0;
						bool dirty = false;

						UI::PropertyLabel("Min Filter");
						if (ImGui::Combo("##MinFilter", &minF, minFilterTypes, 4)) { desc.minFilter = static_cast<MinFilter>(minF); dirty = true; }

						UI::PropertyLabel("Mag Filter");
						if (ImGui::Combo("##MagFilter", &magF, magFilterTypes, 2)) { desc.magFilter = static_cast<MagFilter>(magF); dirty = true; }

						UI::PropertyLabel("Wrap Mode");
						if (ImGui::Combo("##WrapMode", &wrap, wrapTypes, 5)) { desc.wrap = static_cast<Wrap>(wrap); dirty = true; }

						UI::PropertyLabel("Anisotropy");
						if (ImGui::Combo("##AnisoMode", &aniso, anisoLevels, 5))
						{
							constexpr float values[] = { 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
							desc.anisotropy = values[aniso];
							dirty = true;
						}

						if (dirty) material.GetSampler().Init(desc);

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Billboard##billboardhdr"))
				{
					if (UI::BeginPropertyTable("BillboardTable"))
					{
						UI::PropertyLabel("Mode");
						const char* billboardModes[] = { "Disabled", "Spherical", "Cylindrical" };
						int currentBillboardMode = static_cast<int>(material.GetBillboardMode());
						float billW = inst.IsOverridden(MatField::BillboardMode) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(billW);
						if (ImGui::Combo("##bmode", &currentBillboardMode, billboardModes, 3))
							material.SetBillboardMode(static_cast<BillboardMode>(currentBillboardMode));
						if (inst.IsOverridden(MatField::BillboardMode)) RevertButton("R##bmode", [&]() { material.ClearBillboardMode(); });

						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("UV##uvhdr"))
				{
					if (UI::BeginPropertyTable("UVTable"))
					{
						UI::PropertyLabel("UV Offset");
						glm::vec2 uvOffset = material.GetUVOffset();
						float offW = inst.IsOverridden(MatField::UVOffset) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(offW);
						if (ImGui::DragFloat2("##uvoff", &uvOffset[0], 0.05f)) material.SetUVOffset(uvOffset);
						if (inst.IsOverridden(MatField::UVOffset)) RevertButton("R##uvoff", [&]() { material.ClearUVOffset(); });

						UI::PropertyLabel("UV Scale");
						glm::vec2 uvScale = material.GetUVScale();
						float sclW = inst.IsOverridden(MatField::UVScale) ? ImGui::GetContentRegionAvail().x - 26.0f : -FLT_MIN;
						ImGui::SetNextItemWidth(sclW);
						if (ImGui::DragFloat2("##uvscl", &uvScale[0], 0.05f)) material.SetUVScale(uvScale);
						if (inst.IsOverridden(MatField::UVScale)) RevertButton("R##uvscl", [&]() { material.ClearUVScale(); });

						UI::EndPropertyTable();
					}
				}
			});
	}

	DrawComponent<SceneTreeComponent>("Hierarchy", registry, selectedEntity, [&](SceneTreeComponent& selectedNode)
		{
			if (UI::BeginPropertyTable("HierarchyTable"))
			{
				UI::PropertyLabel("Parent Entity");
				std::string parentName = (selectedNode.parent == entt::null) ? "None" : registry.get<TagComponent>(selectedNode.parent).tag + "##" + std::to_string((uint32_t)selectedNode.parent);
				if (ImGui::BeginCombo("##ParentCombo", parentName.c_str()))
				{
					if (ImGui::Selectable("None", selectedNode.parent == entt::null))
						scene.RemoveParent(selectedEntity);

					registry.view<SceneTreeComponent, TagComponent>().each([&](entt::entity entity, SceneTreeComponent&, TagComponent& tag)
						{
							if (entity != selectedEntity)
							{
								std::string displayName = tag.tag + "##" + std::to_string((uint32_t)entity);
								if (ImGui::Selectable(displayName.c_str(), selectedNode.parent == entity))
									scene.SetParent(selectedEntity, entity);
							}
						});
					ImGui::EndCombo();
				}
				UI::EndPropertyTable();
			}
		}, false);

	DrawComponent<ScriptComponent>("Script", registry, selectedEntity, [](ScriptComponent& scriptComp)
		{
			UI::DrawFullWidthAssetDropSlot("Script Slot", "", [&](const std::string& pathStr)
				{
					if (pathStr.ends_with(".lua"))
					{
						AssetUUID scriptUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
						scriptComp.scriptAssetUUID = scriptUUID;
					}
				});
		});

	DrawComponent<AudioEmitterComponent>("Audio Emitter", registry, selectedEntity, [](AudioEmitterComponent& emitter)
		{
			auto& audio = AudioManager::Get();

			UI::DrawFullWidthAssetDropSlot("Audio Clip", "", [&](const std::string& pathStr)
				{
					if (pathStr.ends_with(".wav") || pathStr.ends_with(".mp3"))
					{
						AssetUUID audioClipUUID = EditorUtils::ReadUUIDFromMeta(pathStr);
						emitter.audioClipUUID = audioClipUUID;
					}
				});

			ImGui::Spacing();

			if (UI::BeginPropertyTable("AudioClipTable"))
			{
				auto& busses = audio.GetBusses();
				AudioBus* currentBus = audio.GetBus(emitter.busID);
				if (!currentBus)
				{
					audio.SetBus(&emitter, 0);
					currentBus = audio.GetBus(emitter.busID);
				}

				std::string currentBusName = currentBus ? currentBus->GetName() : "Master";
				for (const auto& bus : busses)
				{
					if (bus->GetID() == emitter.busID)
					{
						currentBusName = bus->GetName();
						break;
					}
				}

				UI::PropertyLabel("Output Bus");
				if (ImGui::BeginCombo("##OutputBus", currentBusName.c_str()))
				{
					for (const auto& bus : busses)
					{
						bool isSelected = (emitter.busID == bus->GetID());
						if (ImGui::Selectable(bus->GetName().c_str(), isSelected))
							audio.SetBus(&emitter, bus->GetID());
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				UI::PropertyLabel("Playback");
				float playbackBtnWidth = (ImGui::GetContentRegionAvail().x - (2.0f * 4.0f)) / 3.0f;
				if (ImGui::Button("Play", ImVec2(playbackBtnWidth, 22))) audio.Play(&emitter);
				ImGui::SameLine(0, 4.0f);
				if (ImGui::Button("Pause", ImVec2(playbackBtnWidth, 22))) audio.Pause(&emitter);
				ImGui::SameLine(0, 4.0f);
				if (ImGui::Button("Stop", ImVec2(playbackBtnWidth, 22))) audio.Stop(&emitter);

				UI::PropertyLabel("Volume");
				float volume = emitter.volume;
				if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 2.0f, "%.2f")) audio.SetVolume(&emitter, volume);

				UI::PropertyLabel("Pitch");
				float pitch = emitter.pitch;
				if (ImGui::SliderFloat("##Pitch", &pitch, 0.01f, 4.0f, "%.2f")) audio.SetPitch(&emitter, pitch);

				UI::PropertyLabel("Loop");
				bool loop = emitter.loop;
				if (ImGui::Checkbox("##Loop", &loop)) audio.SetLoop(&emitter, loop);

				UI::PropertyLabel("Play On Create");
				ImGui::Checkbox("##PlayOnCreate", &emitter.playOnCreate);

				UI::PropertyLabel("Spatial Audio");
				bool spatial = emitter.spatial;
				if (ImGui::Checkbox("##Spatial", &spatial)) audio.SetSpatial(&emitter, spatial);

				UI::EndPropertyTable();
			}

			if (emitter.spatial)
			{
				if (ImGui::CollapsingHeader("Attenuation"))
				{
					if (UI::BeginPropertyTable("AttenuationTable"))
					{
						UI::PropertyLabel("Distance Model");
						const char* attenuationModels[] = { "None", "Inverse", "Linear", "Exponential" };
						int currentModel = static_cast<int>(emitter.attenuationModel);
						if (ImGui::Combo("##AttModel", &currentModel, attenuationModels, 4))
							audio.SetAttenuationModel(&emitter, static_cast<AttenuationModel>(currentModel));

						if (emitter.attenuationModel != AttenuationModel::None)
						{
							UI::PropertyLabel("Min Distance");
							float minDist = emitter.minDistance;
							if (ImGui::DragFloat("##MinDist", &minDist, 0.1f, 0.01f, emitter.maxDistance - 0.01f, "%.2f"))
								audio.SetMinDistance(&emitter, minDist);

							UI::PropertyLabel("Max Distance");
							float maxDist = emitter.maxDistance;
							if (ImGui::DragFloat("##MaxDist", &maxDist, 0.5f, emitter.minDistance + 0.01f, 10000.0f, "%.2f"))
								audio.SetMaxDistance(&emitter, maxDist);

							if (emitter.attenuationModel == AttenuationModel::Inverse || emitter.attenuationModel == AttenuationModel::Exponential)
							{
								UI::PropertyLabel("Rolloff");
								float rolloff = emitter.rolloff;
								if (ImGui::SliderFloat("##Rolloff", &rolloff, 0.0f, 10.0f, "%.2f"))
									audio.SetRolloff(&emitter, rolloff);
							}
						}
						UI::EndPropertyTable();
					}
				}

				if (ImGui::CollapsingHeader("Doppler Effect"))
				{
					if (UI::BeginPropertyTable("DopplerTable"))
					{
						UI::PropertyLabel("Enabled");
						bool dopplerEnabled = emitter.doppler;
						if (ImGui::Checkbox("##DopplerEnabled", &dopplerEnabled))
						{
							emitter.doppler = dopplerEnabled;
							audio.SetDopplerFactor(&emitter, dopplerEnabled ? emitter.dopplerFactor : 0.0f);
						}

						if (emitter.doppler)
						{
							UI::PropertyLabel("Factor");
							float dopplerFactor = emitter.dopplerFactor;
							if (ImGui::SliderFloat("##DopplerFactor", &dopplerFactor, 0.0f, 5.0f, "%.2f"))
								audio.SetDopplerFactor(&emitter, dopplerFactor);
						}
						UI::EndPropertyTable();
					}
				}
			}
		});

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

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

		DrawAddComponentEntry<ScriptComponent>("Script", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<MaterialComponent>("Material", filterBuf, registry, selectedEntity, AssetUUID::FromString("ee3dde12-6263-4f11-bb1d-812b3e196ab7"));
		DrawAddComponentEntry<MeshComponent>("Mesh", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<MeshRendererComponent>("Mesh Renderer", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<PointLightComponent>("Point Light", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<DirectionalLightComponent>("Directional Light", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<RigidbodyComponent>("Rigidbody", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<CharacterBodyComponent>("CharacterBody", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<BoxColliderComponent>("Box Collider", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<SphereColliderComponent>("Sphere Collider", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<CapsuleColliderComponent>("Capsule Collider", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<CylinderColliderComponent>("Cylinder Collider", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<MeshColliderComponent>("Mesh Collider", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<ConvexHullColliderComponent>("Convex Hull Collider", filterBuf, registry, selectedEntity);
		DrawAddComponentEntry<AudioEmitterComponent>("Audio Emitter", filterBuf, registry, selectedEntity);

		if (!ImGui::IsPopupOpen("AddComponentPopup"))
			filterBuf[0] = '\0';

		ImGui::EndPopup();
	}

	ImGui::End();
}