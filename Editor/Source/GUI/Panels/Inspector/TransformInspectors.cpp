#include "TransformInspectors.h"
#include "InspectorUIUtils.h"
#include "ImGui/imgui.h"
#include "Scene/Entity.h"

namespace rv {

void TransformInspectors::DrawTransform(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<TransformComponent>("Transform", registry, selectedEntity, [](TransformComponent& transform)
	{
		if (UI::BeginPropertyTable("TransformTable"))
		{
			glm::vec3 pos = transform.GetPosition();
			glm::vec3 euler = transform.GetEulerRotation();
			glm::vec3 scale = transform.GetScale();
			bool isScaleRatioLocked = transform.IsScaleRatioLocked();

			bool oldLocked = isScaleRatioLocked;

			if (UI::DrawTransformRow("Position", pos, 0.0f, 0.1f))
			{
				transform.SetPosition(pos);
				transform.SetDirty();
			}

			if (UI::DrawTransformRow("Rotation", euler, 0.0f, 0.1f, -360.0f, 360.0f))
			{
				transform.SetEulerRotation(euler);
				transform.SetDirty();
			}

			if (UI::DrawTransformRow("Scale", scale, 1.0f, 0.05f, 0.001f, 10000.0f, true, &isScaleRatioLocked))
			{
				transform.SetScale(scale);
				transform.SetDirty();
			}

			if (isScaleRatioLocked != oldLocked)
			{
				transform.SetLockScaleRatio(isScaleRatioLocked);
			}

			UI::EndPropertyTable();
		}
	}, false);
}

void TransformInspectors::DrawHierarchy(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	UI::DrawComponent<SceneTreeComponent>("Hierarchy", registry, selectedEntity, [&](SceneTreeComponent& selectedNode)
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
}

void TransformInspectors::Draw(Scene& scene, entt::entity selectedEntity)
{
	DrawTransform(scene, selectedEntity);
	DrawHierarchy(scene, selectedEntity);
}

} // namespace rv
