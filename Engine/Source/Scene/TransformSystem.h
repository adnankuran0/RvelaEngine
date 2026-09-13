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
    void InterpolatePhysicsBodies();
    void InterpolateRigidbodies(float alpha);
    void InterpolateCharacterBodies(float alpha);
    void UpdateNodeRecursive(entt::entity e, const glm::mat4& parentWorldMatrix, bool parentDirty);
    entt::entity FindSkeletonEntity(entt::entity e);

    Scene& m_Scene;
};

}