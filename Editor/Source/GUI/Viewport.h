#pragma once
#include "entt/entt.h"
#include "glm/glm.hpp"
#include "ImGui/imgui.h"
#include "ImGui/ImGuizmo.h"

#include <vector>
#include <unordered_map>
#include "glm/gtc/quaternion.hpp"

namespace rv {

class Engine;

class Viewport
{
public:
	void Draw(Engine* engine);
	bool ContainsPoint(const glm::vec2& point) const;

	float GetSnapTranslate() const { return m_snapTranslate; }
	void SetSnapTranslate(float v) { m_snapTranslate = v; }
	float GetSnapRotate() const { return m_snapRotate; }
	void SetSnapRotate(float v) { m_snapRotate = v; }
	float GetSnapScale() const { return m_snapScale; }
	void SetSnapScale(float v) { m_snapScale = v; }
	bool GetEnableSnap() const { return m_EnableSnap; }
	void SetEnableSnap(bool v) { m_EnableSnap = v; }
	ImGuizmo::MODE GetGizmoMode() const { return m_CurrentGizmoMode; }
	void SetGizmoMode(ImGuizmo::MODE m) { m_CurrentGizmoMode = m; }

private:
	void DrawGizmos(Engine* engine, ImVec2& displayPos, ImVec2& displaySize);
	void DrawToolbar(Engine* engine, ImVec2& displayPos, ImVec2& displaySize);
	void DrawOverlayStats(Engine* engine, ImVec2& displayPos, ImVec2& displaySize);
	void DrawOrientationGizmo(Engine* engine, ImVec2& displayPos, ImVec2& displaySize);
	void HandleSelection(Engine* engine, ImVec2& displayPos, ImVec2& displaySize);

private:
	float m_snapTranslate = 1.0f;
	float m_snapRotate = 15.0f;
	float m_snapScale = 0.5f;
	bool m_EnableSnap = false;

	ImGuizmo::OPERATION m_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE m_CurrentGizmoMode = ImGuizmo::WORLD;

	struct InitialTransform {
		glm::mat4 worldMatrix{ 1.0f };
		glm::vec3 worldPos{ 0.0f };
		glm::quat worldRot{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 worldScale{ 1.0f };
	};

	bool m_WasGizmoUsing = false;
	ImVec2 m_PanelPosition{ 0.0f, 0.0f };
	ImVec2 m_PanelSize{ 0.0f, 0.0f };
	std::unordered_map<entt::entity, InitialTransform> m_InitialTransforms;
	glm::mat4 m_ActiveGizmoMatrix{ 1.0f };
	glm::mat4 m_InitialGizmoMatrix{ 1.0f };
	glm::vec3 m_InitialGizmoPos{ 0.0f };
	glm::quat m_InitialGizmoRot{ 1.0f, 0.0f, 0.0f, 0.0f };
	glm::vec3 m_InitialGizmoScale{ 1.0f };
};

}
