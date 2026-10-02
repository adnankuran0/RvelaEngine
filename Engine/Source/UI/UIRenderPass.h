#pragma once

#include <glm/glm.hpp>
#include <entt/entt.h>
#include <memory>

namespace rv {

class Scene;
class Shader;

class UIRenderPass
{
public:
    static void Render(Scene* scene, float viewportWidth, float viewportHeight);
    static void RenderEntityIDs(Scene* scene, float viewportWidth, float viewportHeight);
    static void RenderSelectionMask(Scene* scene, entt::entity selectedEntity, float viewportWidth, float viewportHeight);

private:
    static void RenderEntityRecursively(Scene* scene, entt::entity entity);
    static void RenderEntityIDRecursively(Scene* scene, entt::entity entity, std::shared_ptr<Shader> shader);
};

}
