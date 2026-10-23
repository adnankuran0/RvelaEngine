#include "PhysicsInspectors.h"
#include "InspectorUIUtils.h"
#include "ImGui/imgui.h"
#include "Scene/Entity.h"
#include "Scene/Components/PhysicsConstraintComponent.h"
#include "Scene/Components/TagComponent.h"
#include "Scene/Components/UUIDComponent.h"
#include "Scene/Components/CharacterBodyComponent.h"
#include "Utils/ProjectManager.h"
#include <algorithm>
#include <string>

namespace rv {

void PhysicsInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<RigidbodyComponent>("Rigidbody", registry, selectedEntity, [](RigidbodyComponent& rb)
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

				auto activeProj = ProjectManager::GetActiveProject();

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

					if (ImGui::IsItemHovered())
					{
						std::string layerName;
						if (activeProj)
							layerName = activeProj->GetSettings().GetPhysicsLayerNameByBit(i);
						if (!layerName.empty())
							ImGui::SetTooltip("Layer %d: %s", i + 1, layerName.c_str();
						else
							ImGui::SetTooltip("Layer %d", i + 1);
					}

					ImGui::PopID();
					ImGui::PopStyleColor();

					if ((i % 8) != 7)
						ImGui::SameLine(0, 4.0f);
				}

				if (activeProj)
				{
					std::string treeId = std::string("Named Layers##") + headerTitle;
					if (ImGui::TreeNode(treeId.c_str()))
					{
						const auto& pSettings = activeProj->GetSettings();
						for (int i = 0; i < 32; i++)
						{
							const std::string& name = pSettings.GetPhysicsLayerNameByBit(i);
							if (!name.empty())
							{
								bool active = (bitField & (1u << i)) != 0;
								std::string itemLabel = std::to_string(i + 1) + ": " + name + "##" + headerTitle + std::to_string(i);
								if (ImGui::Checkbox(itemLabel.c_str(), &active))
								{
									if (active) bitField |= (1u << i);
									else bitField &= ~(1u << i);
								}
							}
						}
						ImGui::TreePop();
					}
				}
			};

			DrawBitGrid("Layer Mask", rb.collisionFilter.layer);
			ImGui::Spacing();
			DrawBitGrid("Collision Mask", rb.collisionFilter.mask);
		}
	});

	UI::DrawComponent<CharacterBodyComponent>("CharacterBody", registry, selectedEntity, [](CharacterBodyComponent& cb)
	{
		if (UI::BeginPropertyTable("CharacterBodyTable"))
		{
			float maxSlope = cb.maxSlopeAngle * (180.0f / 3.14159265f);
			UI::PropertyLabel("Max Slope Angle");
			if (ImGui::SliderFloat("##MaxSlope", &maxSlope, 0.0f, 89.0f, "%.1f deg"))
				cb.maxSlopeAngle = maxSlope * (3.14159265f / 180.0f);

			UI::EndPropertyTable();
		}

		ImGui::Spacing();
		if (ImGui::CollapsingHeader("Collision Filter##CB"))
		{
			auto DrawBitGridCB = [](const char* headerTitle, uint32_t& bitField)
			{
				ImGui::TextDisabled("%s", headerTitle);
				float btnSize = (ImGui::GetContentRegionAvail().x - (7.0f * 4.0f)) / 8.0f;
				btnSize = std::max(btnSize, 18.0f);

				auto activeProj = ProjectManager::GetActiveProject();

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

					if (ImGui::IsItemHovered())
					{
						std::string layerName;
						if (activeProj)
							layerName = activeProj->GetSettings().GetPhysicsLayerNameByBit(i);
						if (!layerName.empty())
							ImGui::SetTooltip("Layer %d: %s", i + 1, layerName.c_str());
						else
							ImGui::SetTooltip("Layer %d", i + 1);
					}

					ImGui::PopID();
					ImGui::PopStyleColor();

					if ((i % 8) != 7)
						ImGui::SameLine(0, 4.0f);
				}
			};

			DrawBitGridCB("Layer Mask##CBLayer", cb.collisionFilter.layer);
			ImGui::Spacing();
			DrawBitGridCB("Collision Mask##CBMask", cb.collisionFilter.mask);
		}
	});

	UI::DrawComponent<BoxColliderComponent>("Box Collider", registry, selectedEntity, [](BoxColliderComponent& bc)
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

	UI::DrawComponent<SphereColliderComponent>("Sphere Collider", registry, selectedEntity, [](SphereColliderComponent& sc)
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

	UI::DrawComponent<CapsuleColliderComponent>("Capsule Collider", registry, selectedEntity, [](CapsuleColliderComponent& cc)
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

	UI::DrawComponent<CylinderColliderComponent>("Cylinder Collider", registry, selectedEntity, [](CylinderColliderComponent& cc)
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

	UI::DrawComponent<MeshColliderComponent>("Mesh Collider", registry, selectedEntity, [](MeshColliderComponent& mc)
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

	UI::DrawComponent<ConvexHullColliderComponent>("Convex Hull Collider", registry, selectedEntity, [](ConvexHullColliderComponent& chc)
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

	UI::DrawComponent<PhysicsConstraintComponent>("Physics Constraint", registry, selectedEntity, [&](PhysicsConstraintComponent& pc)
	{
		if (UI::BeginPropertyTable("PhysicsConstraintTable"))
		{
			UI::PropertyLabel("Constraint Type");
			const char* types[] = { "Fixed", "Point", "Distance", "Hinge", "Slider", "Cone" };
			int currentType = static_cast<int>(pc.type);
			if (ImGui::Combo("##ConstraintType", &currentType, types, 6))
			{
				pc.type = static_cast<ConstraintType>(currentType);
				pc.SetDirty();
			}

			UI::PropertyLabel("Enabled");
			if (ImGui::Checkbox("##ConstraintEnabled", &pc.enabled))
			{
				pc.SetDirty();
			}

			UI::PropertyLabel("Connected Entity");
			std::string connectedName = "World (Static)";
			if (pc.connectedEntityUUID != 0)
			{
				connectedName = "UUID: " + std::to_string(pc.connectedEntityUUID).substr(0, 8) + "...";
				auto uuidView = registry.view<UUIDComponent>();
				for (auto e : uuidView)
				{
					if (uuidView.get<UUIDComponent>(e).uuid == pc.connectedEntityUUID)
					{
						if (auto* tag = registry.try_get<TagComponent>(e))
							connectedName = tag->tag;
						break;
					}
				}
			}

			const float totalAvail = ImGui::GetContentRegionAvail().x;
			constexpr float clearButtonWidth = 22.0f;
			const float slotWidth = std::max(totalAvail - clearButtonWidth - 4.0f, 40.0f);
			const std::string slotId = connectedName + "##ConnectedEntitySlot";

			bool openPopup = false;
			if (ImGui::Button(slotId.c_str(), ImVec2(slotWidth, 22.0f)))
			{
				openPopup = true;
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG"))
				{
					if (payload->DataSize == sizeof(entt::entity))
					{
						const entt::entity dragged = *static_cast<const entt::entity*>(payload->Data);
						if (registry.valid(dragged) && registry.any_of<UUIDComponent>(dragged))
						{
							if (dragged != selectedEntity)
							{
								pc.connectedEntityUUID = registry.get<UUIDComponent>(dragged).uuid;
								pc.SetDirty();
							}
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine(0, 4.0f);
			if (ImGui::Button("X##ClearConnectedEntity", ImVec2(clearButtonWidth, 22.0f)))
			{
				pc.connectedEntityUUID = 0;
				pc.SetDirty();
			}

			if (openPopup)
			{
				ImGui::OpenPopup("SelectConnectedEntityPopup");
			}

			if (ImGui::BeginPopup("SelectConnectedEntityPopup"))
			{
				ImGui::TextDisabled("Select Connected Entity");
				ImGui::Separator();

				if (ImGui::Selectable("World (Static)", pc.connectedEntityUUID == 0))
				{
					pc.connectedEntityUUID = 0;
					pc.SetDirty();
				}

				ImGui::Separator();
				auto bodyView = registry.view<UUIDComponent, TagComponent>();
				for (auto e : bodyView)
				{
					if (e == selectedEntity) continue;

					const auto& uuidComp = bodyView.get<UUIDComponent>(e);
					const auto& tagComp = bodyView.get<TagComponent>(e);

					bool isSelected = (pc.connectedEntityUUID == uuidComp.uuid);
					std::string label = tagComp.tag + "##" + std::to_string(uuidComp.uuid);
					if (ImGui::Selectable(label.c_str(), isSelected))
					{
						pc.connectedEntityUUID = uuidComp.uuid;
						pc.SetDirty();
					}
				}
				ImGui::EndPopup();
			}

			UI::PropertyLabel("Space");
			const char* spaces[] = { "World Space", "Local Space" };
			int currentSpace = static_cast<int>(pc.space);
			if (ImGui::Combo("##ConstraintSpace", &currentSpace, spaces, 2))
			{
				pc.space = static_cast<ConstraintSpace>(currentSpace);
				pc.SetDirty();
			}

			if (pc.type == ConstraintType::Fixed || pc.type == ConstraintType::Slider)
			{
				UI::PropertyLabel("Auto Detect Point");
				if (ImGui::Checkbox("##AutoDetectPoint", &pc.autoDetectPoints))
				{
					pc.SetDirty();
				}
			}

			if (!pc.autoDetectPoints || (pc.type != ConstraintType::Fixed && pc.type != ConstraintType::Slider))
			{
				UI::DrawVec3Control("Anchor A", pc.anchorA, 0.0f, 0.05f);
				UI::DrawVec3Control("Anchor B", pc.anchorB, 0.0f, 0.05f);
			}

			if (pc.type == ConstraintType::Hinge || pc.type == ConstraintType::Slider || pc.type == ConstraintType::Cone)
			{
				UI::DrawVec3Control("Axis A", pc.axisA, 0.0f, 0.05f);
				UI::DrawVec3Control("Axis B", pc.axisB, 0.0f, 0.05f);
			}

			if (pc.type == ConstraintType::Distance)
			{
				UI::PropertyLabel("Min Distance");
				if (ImGui::DragFloat("##MinDist", &pc.minLimit, 0.05f, -1.0f, 1000.0f, "%.2f")) pc.SetDirty();
				UI::PropertyLabel("Max Distance");
				if (ImGui::DragFloat("##MaxDist", &pc.maxLimit, 0.05f, -1.0f, 1000.0f, "%.2f")) pc.SetDirty();
			}
			else if (pc.type == ConstraintType::Hinge)
			{
				UI::PropertyLabel("Min Angle");
				if (ImGui::SliderAngle("##MinAngle", &pc.minLimit, -180.0f, 0.0f)) pc.SetDirty();
				UI::PropertyLabel("Max Angle");
				if (ImGui::SliderAngle("##MaxAngle", &pc.maxLimit, 0.0f, 180.0f)) pc.SetDirty();
			}
			else if (pc.type == ConstraintType::Slider)
			{
				UI::PropertyLabel("Min Limit");
				if (ImGui::DragFloat("##MinSliderLimit", &pc.minLimit, 0.05f, -1000.0f, 0.0f, "%.2f")) pc.SetDirty();
				UI::PropertyLabel("Max Limit");
				if (ImGui::DragFloat("##MaxSliderLimit", &pc.maxLimit, 0.05f, 0.0f, 1000.0f, "%.2f")) pc.SetDirty();
			}
			else if (pc.type == ConstraintType::Cone)
			{
				UI::PropertyLabel("Half Angle");
				if (ImGui::SliderAngle("##HalfAngle", &pc.maxLimit, 0.0f, 180.0f)) pc.SetDirty();
			}

			if (pc.type == ConstraintType::Hinge || pc.type == ConstraintType::Slider)
			{
				UI::PropertyLabel("Max Friction");
				if (ImGui::DragFloat("##MaxFriction", &pc.maxFriction, 0.1f, 0.0f, 1000.0f, "%.2f")) pc.SetDirty();

				UI::PropertyLabel("Motor Enabled");
				if (ImGui::Checkbox("##MotorEnabled", &pc.motorEnabled)) pc.SetDirty();

				if (pc.motorEnabled)
				{
					UI::PropertyLabel("Target Velocity");
					if (ImGui::DragFloat("##TargetVelocity", &pc.motorTargetVelocity, 0.1f)) pc.SetDirty();
					UI::PropertyLabel("Max Motor Force");
					if (ImGui::DragFloat("##MaxMotorForce", &pc.motorMaxForce, 0.5f, 0.0f, 10000.0f, "%.1f")) pc.SetDirty();
				}
			}

			if (pc.type == ConstraintType::Distance || pc.type == ConstraintType::Hinge || pc.type == ConstraintType::Slider)
			{
				UI::PropertyLabel("Spring Frequency");
				if (ImGui::DragFloat("##SpringFreq", &pc.springFrequency, 0.1f, 0.0f, 50.0f, "%.2f")) pc.SetDirty();
				UI::PropertyLabel("Spring Damping");
				if (ImGui::DragFloat("##SpringDamp", &pc.springDamping, 0.05f, 0.0f, 10.0f, "%.2f")) pc.SetDirty();
			}

			UI::EndPropertyTable();
		}
	});
}

} // namespace rv
