#pragma once
#include "entt/entt.h"
#include "glm/glm.hpp"
#include "ImGui/imgui.h"
#include "ImGui/ImGuizmo.h"

namespace rv {

class Engine;

class Viewport
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity);
private:
	void DrawGizmos(Engine* engine, ImVec2& displayPos, ImVec2& displaySize, entt::entity selectedEntity);
	void DrawPopups(Engine* engine, ImVec2& displayPos, ImVec2& displaySize);
	void HandleSelection(Engine* engine, ImVec2& displayPos, ImVec2& displaySize, entt::entity& selectedEntity);
private:
	float m_snapTranslate = 1.0f;
	float m_snapRotate = 15.0f;
	float m_snapScale = 0.5f;
	ImGuizmo::OPERATION m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE m_CurrentGizmoMode = ImGuizmo::WORLD;
};

}