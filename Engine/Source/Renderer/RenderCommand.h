#pragma once
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/SkeletalMeshRendererComponent.h"
#include "Scene/Components/SkeletonComponent.h"
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

struct SkeletalRenderCommand
{
    entt::entity entityID = entt::null;
    const TransformComponent* transform = nullptr;
    const SkeletalMeshRendererComponent* mesh = nullptr;
    const SkeletonComponent* skeleton = nullptr;
    MaterialComponent* material = nullptr;
    float distanceToCamera = 0.0f;
};

struct RenderCommand
{
public:
    entt::entity entityID = entt::null;
    const TransformComponent* transform = nullptr;
    const MeshRendererComponent* mesh = nullptr;
    MaterialComponent* material = nullptr;
    float distanceToCamera = 0.0f;
};

}