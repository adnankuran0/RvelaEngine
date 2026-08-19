#pragma once
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "entt/entt.h"

namespace rv {

struct ParticleRenderCommand {
    MeshRendererComponent* mesh;
    MaterialComponent* material;
    uint32_t instanceOffset;
    uint32_t instanceCount;
    bool localCoords;
    glm::vec3 worldPosition;
    float distanceToCamera;
    int indexCount;
};

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