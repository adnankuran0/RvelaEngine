#include "AddComponentPopup.h"
#include "InspectorUIUtils.h"
#include "ImGui/imgui.h"
#include "Scene/Entity.h"
#include "Scene/Components/SkeletonComponent.h"
#include "Scene/Components/CameraComponent.h"
#include "GUI/Style.h"
#include "Scene/Components/NavMeshSurfaceComponent.h"
#include "Scene/Components/NavMeshAgentComponent.h"
#include "Scene/Components/NavMeshObstacleComponent.h"
#include <algorithm>
#include <string>

namespace rv {

template<typename T>
static void AddComponentToEntity(const char* label, entt::registry& registry, entt::entity entity)
{
	registry.emplace<T>(entity);
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
}

template<typename T>
static bool DrawComponentMenuItem(const char* label, const char* category, const char* searchFilter, entt::registry& registry, entt::entity entity)
{
	bool alreadyHas = registry.any_of<T>(entity);

	if (searchFilter && searchFilter[0] != '\0')
	{
		std::string filterStr = searchFilter;
		std::string labelStr = label;
		std::string catStr = category ? category : "";
		auto toLower = [](std::string& s) {
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		};
		toLower(filterStr);
		toLower(labelStr);
		toLower(catStr);

		if (labelStr.find(filterStr) == std::string::npos && catStr.find(filterStr) == std::string::npos)
			return false;

		const char* tag = alreadyHas ? "[Added]" : category;
		if (ImGui::MenuItem(label, tag, false, !alreadyHas))
		{
			AddComponentToEntity<T>(label, registry, entity);
			ImGui::CloseCurrentPopup();
		}
		if (alreadyHas && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Already added to this entity");
		}
		return true;
	}
	else
	{
		const char* tag = alreadyHas ? "[Added]" : nullptr;
		if (ImGui::MenuItem(label, tag, false, !alreadyHas))
		{
			AddComponentToEntity<T>(label, registry, entity);
			ImGui::CloseCurrentPopup();
		}
		if (alreadyHas && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Already added to this entity");
		}
		return true;
	}
}

void AddComponentPopup::Draw(Scene& scene, entt::entity selectedEntity)
{
	entt::registry& registry = scene.GetRegistry();

	float availWidth = ImGui::GetContentRegionAvail().x;
	float btnWidth = std::clamp(availWidth * 0.85f, 160.0f, 220.0f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - btnWidth) * 0.5f);

	ImVec4 acc = GetAccentColor();
	ImVec4 accHov = GetAccentHoverColor();
	ImVec4 accAct = GetAccentActiveColor();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(acc.x, acc.y, acc.z, 0.18f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(accHov.x, accHov.y, accHov.z, 0.35f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(accAct.x, accAct.y, accAct.z, 0.50f));
	ImGui::PushStyleColor(ImGuiCol_Text, acc);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));

	if (ImGui::Button("+ Add Component", ImVec2(btnWidth, 30.0f)))
		ImGui::OpenPopup("AddComponentPopup");

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(4);

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		static char filterBuf[128] = "";
		ImGui::SetNextItemWidth(220.0f);
		if (ImGui::IsWindowAppearing())
			ImGui::SetKeyboardFocusHere();
		ImGui::InputTextWithHint("##Filter", "Search Components...", filterBuf, sizeof(filterBuf));
		ImGui::Separator();

		if (filterBuf[0] == '\0')
		{
			if (ImGui::BeginMenu("Physics"))
			{
				DrawComponentMenuItem<RigidbodyComponent>("Rigidbody", "Physics", "", registry, selectedEntity);
				DrawComponentMenuItem<CharacterBodyComponent>("CharacterBody", "Physics", "", registry, selectedEntity);
				DrawComponentMenuItem<PhysicsConstraintComponent>("Physics Constraint", "Physics", "", registry, selectedEntity);
				ImGui::Separator();
				DrawComponentMenuItem<BoxColliderComponent>("Box Collider", "Physics", "", registry, selectedEntity);
				DrawComponentMenuItem<SphereColliderComponent>("Sphere Collider", "Physics", "", registry, selectedEntity);
				DrawComponentMenuItem<CapsuleColliderComponent>("Capsule Collider", "Physics", "", registry, selectedEntity);
				DrawComponentMenuItem<CylinderColliderComponent>("Cylinder Collider", "Physics", "", registry, selectedEntity);
				DrawComponentMenuItem<MeshColliderComponent>("Mesh Collider", "Physics", "", registry, selectedEntity);
				DrawComponentMenuItem<ConvexHullColliderComponent>("Convex Hull Collider", "Physics", "", registry, selectedEntity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("UI"))
			{
				DrawComponentMenuItem<UICanvasComponent>("UI Canvas", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<RectTransformComponent>("Rect Transform", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UIPanelComponent>("UI Panel", "UI", "", registry, selectedEntity);
				ImGui::Separator();
				DrawComponentMenuItem<UIImageComponent>("UI Image", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UITextComponent>("UI Text", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UIButtonComponent>("UI Button", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UISliderComponent>("UI Slider", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UIProgressBarComponent>("UI Progress Bar", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UICheckboxComponent>("UI Checkbox", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UITextInputComponent>("UI Text Input", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UIScrollRectComponent>("UI Scroll Rect", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UITabContainerComponent>("UI Tab Container", "UI", "", registry, selectedEntity);
				DrawComponentMenuItem<UILayoutGroupComponent>("UI Layout Group", "UI", "", registry, selectedEntity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Rendering"))
			{
				DrawComponentMenuItem<CameraComponent>("Camera", "Rendering", "", registry, selectedEntity);
				DrawComponentMenuItem<MeshComponent>("Mesh", "Rendering", "", registry, selectedEntity);
				DrawComponentMenuItem<MeshRendererComponent>("Mesh Renderer", "Rendering", "", registry, selectedEntity);
				DrawComponentMenuItem<MaterialComponent>("Material", "Rendering", "", registry, selectedEntity);
				DrawComponentMenuItem<DecalComponent>("Decal", "Rendering", "", registry, selectedEntity);
				ImGui::Separator();
				DrawComponentMenuItem<SkeletalMeshComponent>("Skeletal Mesh", "Rendering", "", registry, selectedEntity);
				DrawComponentMenuItem<SkeletalMeshRendererComponent>("Skeletal Mesh Renderer", "Rendering", "", registry, selectedEntity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Lighting"))
			{
				DrawComponentMenuItem<DirectionalLightComponent>("Directional Light", "Lighting", "", registry, selectedEntity);
				DrawComponentMenuItem<PointLightComponent>("Point Light", "Lighting", "", registry, selectedEntity);
				DrawComponentMenuItem<SpotLightComponent>("Spot Light", "Lighting", "", registry, selectedEntity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Audio"))
			{
				DrawComponentMenuItem<AudioEmitterComponent>("Audio Emitter", "Audio", "", registry, selectedEntity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Animation"))
			{
				DrawComponentMenuItem<AnimatorComponent>("Animator", "Animation", "", registry, selectedEntity);
				DrawComponentMenuItem<SkeletonComponent>("Skeleton", "Animation", "", registry, selectedEntity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Effects"))
			{
				DrawComponentMenuItem<ParticleEmitterComponent>("Particle Emitter", "Effects", "", registry, selectedEntity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Scripts & Logic"))
			{
				DrawComponentMenuItem<ScriptComponent>("Script", "Scripts", "", registry, selectedEntity);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Navigation"))
			{
				DrawComponentMenuItem<NavMeshSurfaceComponent>("NavMesh Surface", "Navigation", "", registry, selectedEntity);
				DrawComponentMenuItem<NavMeshAgentComponent>("NavMesh Agent", "Navigation", "", registry, selectedEntity);
				DrawComponentMenuItem<NavMeshObstacleComponent>("NavMesh Obstacle", "Navigation", "", registry, selectedEntity);
				ImGui::EndMenu();
			}
		}
		else
		{
			int matchCount = 0;

			#define RV_FILTER_ITEM(Type, Label, Category) \
				if (DrawComponentMenuItem<Type>(Label, Category, filterBuf, registry, selectedEntity)) matchCount++

			// Physics
			RV_FILTER_ITEM(RigidbodyComponent, "Rigidbody", "Physics");
			RV_FILTER_ITEM(CharacterBodyComponent, "CharacterBody", "Physics");
			RV_FILTER_ITEM(PhysicsConstraintComponent, "Physics Constraint", "Physics");
			RV_FILTER_ITEM(BoxColliderComponent, "Box Collider", "Physics");
			RV_FILTER_ITEM(SphereColliderComponent, "Sphere Collider", "Physics");
			RV_FILTER_ITEM(CapsuleColliderComponent, "Capsule Collider", "Physics");
			RV_FILTER_ITEM(CylinderColliderComponent, "Cylinder Collider", "Physics");
			RV_FILTER_ITEM(MeshColliderComponent, "Mesh Collider", "Physics");
			RV_FILTER_ITEM(ConvexHullColliderComponent, "Convex Hull Collider", "Physics");

			// UI
			RV_FILTER_ITEM(UICanvasComponent, "UI Canvas", "UI");
			RV_FILTER_ITEM(RectTransformComponent, "Rect Transform", "UI");
			RV_FILTER_ITEM(UIPanelComponent, "UI Panel", "UI");
			RV_FILTER_ITEM(UIImageComponent, "UI Image", "UI");
			RV_FILTER_ITEM(UITextComponent, "UI Text", "UI");
			RV_FILTER_ITEM(UIButtonComponent, "UI Button", "UI");
			RV_FILTER_ITEM(UISliderComponent, "UI Slider", "UI");
			RV_FILTER_ITEM(UIProgressBarComponent, "UI Progress Bar", "UI");
			RV_FILTER_ITEM(UICheckboxComponent, "UI Checkbox", "UI");
			RV_FILTER_ITEM(UITextInputComponent, "UI Text Input", "UI");
			RV_FILTER_ITEM(UIScrollRectComponent, "UI Scroll Rect", "UI");
			RV_FILTER_ITEM(UITabContainerComponent, "UI Tab Container", "UI");
			RV_FILTER_ITEM(UILayoutGroupComponent, "UI Layout Group", "UI");

			// Rendering
			RV_FILTER_ITEM(CameraComponent, "Camera", "Rendering");
			RV_FILTER_ITEM(MeshComponent, "Mesh", "Rendering");
			RV_FILTER_ITEM(MeshRendererComponent, "Mesh Renderer", "Rendering");
			RV_FILTER_ITEM(MaterialComponent, "Material", "Rendering");
			RV_FILTER_ITEM(DecalComponent, "Decal", "Rendering");
			RV_FILTER_ITEM(SkeletalMeshComponent, "Skeletal Mesh", "Rendering");
			RV_FILTER_ITEM(SkeletalMeshRendererComponent, "Skeletal Mesh Renderer", "Rendering");

			// Lighting
			RV_FILTER_ITEM(DirectionalLightComponent, "Directional Light", "Lighting");
			RV_FILTER_ITEM(PointLightComponent, "Point Light", "Lighting");
			RV_FILTER_ITEM(SpotLightComponent, "Spot Light", "Lighting");

			// Audio
			RV_FILTER_ITEM(AudioEmitterComponent, "Audio Emitter", "Audio");

			// Animation
			RV_FILTER_ITEM(AnimatorComponent, "Animator", "Animation");
			RV_FILTER_ITEM(SkeletonComponent, "Skeleton", "Animation");

			// Effects
			RV_FILTER_ITEM(ParticleEmitterComponent, "Particle Emitter", "Effects");

			// Scripts
			RV_FILTER_ITEM(ScriptComponent, "Script", "Scripts");

			// Navigation
			RV_FILTER_ITEM(NavMeshSurfaceComponent, "NavMesh Surface", "Navigation");
			RV_FILTER_ITEM(NavMeshAgentComponent, "NavMesh Agent", "Navigation");
			RV_FILTER_ITEM(NavMeshObstacleComponent, "NavMesh Obstacle", "Navigation");

			#undef RV_FILTER_ITEM

			if (matchCount == 0)
			{
				ImGui::TextDisabled("No matching components found");
			}
		}

		if (!ImGui::IsPopupOpen("AddComponentPopup"))
			filterBuf[0] = '\0';

		ImGui::EndPopup();
	}
}

} // namespace rv
