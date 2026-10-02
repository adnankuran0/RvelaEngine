#pragma once

#include <glm/glm.hpp>
#include <entt/entt.h>
#include <vector>
#include "Scene/Components/UIComponents.h"

namespace rv {

class Scene;

class UISystem
{
public:
    UISystem() = default;
    ~UISystem() = default;

    void Init();
    void Shutdown();

    void Update(Scene* scene, float dt, const glm::vec2& viewportSize, const glm::vec2& mousePos, bool mousePressed, bool mouseHeld, bool mouseReleased);
    void Render(Scene* scene, float viewportWidth, float viewportHeight);
    void RenderEntityIDs(Scene* scene, float viewportWidth, float viewportHeight);
    void RenderSelectionMask(Scene* scene, entt::entity selectedEntity, float viewportWidth, float viewportHeight);

private:
    void UpdateLayoutRecursively(Scene* scene, entt::entity entity, const glm::vec2& parentPos, const glm::vec2& parentSize, const glm::vec2& parentCenter, float parentRotation);

    void DispatchLuaCallback(Scene* scene, entt::entity entity, const std::string& callbackName, float param = 0.0f);
};

}
