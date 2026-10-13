#include "PhysicsInspectors.h"
#include "InspectorUIUtils.h"
#include "ImGui/imgui.h"
#include "Scene/Entity.h"
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
}

} // namespace rv
