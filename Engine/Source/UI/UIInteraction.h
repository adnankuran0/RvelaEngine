#pragma once

#include <glm/glm.hpp>
#include <entt/entt.h>
#include <string>

namespace rv {

class Scene;

class UIInteraction
{
public:
    static void UpdateInteractions(Scene* scene, float dt, const glm::vec2& viewportSize, const glm::vec2& mousePos, bool mousePressed, bool mouseHeld, bool mouseReleased);
    static void DispatchLuaCallback(Scene* scene, entt::entity entity, const std::string& callbackName, float param = 0.0f);
};

}
