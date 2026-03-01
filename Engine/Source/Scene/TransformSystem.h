#pragma once
#include "entt/entt.h"

namespace rv {

class Scene;

class TransformSystem
{
public:
    TransformSystem(Scene& scene) : m_Scene(scene) {}
    void Update();
private:
    void UpdateNodeRecursive(entt::entity entity,
        const glm::mat4& parentWorld);
    Scene& m_Scene;
};

}