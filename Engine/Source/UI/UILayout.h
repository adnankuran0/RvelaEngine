#pragma once

#include <glm/glm.hpp>
#include <entt/entt.h>

namespace rv {

class Scene;
class Camera;
struct UICanvasComponent;

class UILayout
{
public:
    static glm::vec2 RotatePoint(const glm::vec2& point, const glm::vec2& pivot, float rotationDegrees);
    static bool IsPointInRect(const glm::vec2& point, const glm::vec4& rect, float rotationDegrees = 0.0f);
    static glm::mat4 CalculateWorldSpaceCanvasMatrix(Scene* scene, entt::entity canvasEntity, const UICanvasComponent& canvas, Camera* camera);
    static void UpdateLayoutRecursively(Scene* scene, entt::entity entity, const glm::vec2& parentPos, const glm::vec2& parentSize, const glm::vec2& parentCenter, float parentRotation);
    static void UpdateCanvasLayouts(Scene* scene, float viewportWidth, float viewportHeight);
};

}
