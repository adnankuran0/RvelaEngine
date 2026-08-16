#pragma once
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "entt/entt.h"

namespace rv {

struct RenderCommand
{
public:
    RenderCommand() = default;

    RenderCommand(const TransformComponent& t, const MeshRendererComponent& m, MaterialComponent& mat, entt::entity entity)
        : transform(&t), mesh(&m), material(&mat), entityID(entity), distanceToCamera(0.0f) {
    }

    entt::entity entityID = entt::null;
    const TransformComponent* transform = nullptr;
    const MeshRendererComponent* mesh = nullptr;
    MaterialComponent* material = nullptr;
    float distanceToCamera = 0.0f;
};

}