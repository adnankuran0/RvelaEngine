#pragma once
#include "ImGui/imgui.h"
#include "Core/Engine.h"

class Viewport
{
public:
	static void Draw(Engine* engine, entt::entity& selectedEntity);
private:
	static bool RayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
		const BoundingBox& box, float& t);
	static glm::vec3 ScreenPosToWorldRay(const glm::vec2& mousePos, const glm::vec2& viewportSize,
		const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);
};