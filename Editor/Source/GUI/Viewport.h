#pragma once
#include "entt/entt.h"
#include "glm/glm.hpp"

class Engine;

class Viewport
{
public:
	void Draw(Engine* engine, entt::entity& selectedEntity);
private:
	static bool RayIntersectsTriangle(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
		const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
		float& t);
	static glm::vec3 ScreenPosToWorldRay(const glm::vec2& mousePos, const glm::vec2& viewportSize,
		const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);
};