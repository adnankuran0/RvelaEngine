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
    void UpdateNodeRecursive(entt::entity e, const glm::mat4& parentWorldMatrix, bool parentDirty);
    Scene& m_Scene;
};

}