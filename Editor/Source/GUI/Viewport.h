#pragma once
#include "entt/entt.h"
#include "glm/glm.hpp"
#include "ImGui/imgui.h"

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
};

}