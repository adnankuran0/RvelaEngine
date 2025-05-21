#include "rvelapch.h"
#include "Scene.h"
#include "Entity.h"
#include "../Core/Time.h"
#include "RvelaLog.h"
#include "../Core/Utils/AssetManager.h"
#include "../Core/Utils/Serializer.h"
#include "../Core/Utils/MaterialManager.h"
#include "../Core/Utils/ProjectManager.h"
#include "UUIDGenerator.h"
#include <glm/gtx/matrix_decompose.hpp>

Scene::Scene() : m_Registry() {}




Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(m_Registry.create(), this);

    entity.AddComponent<TransformComponent>(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    entity.AddComponent<WorldTransformComponent>(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    entity.AddComponent<SceneTreeComponent>();
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<UUIDComponent>(UUIDGenerator::Generate());
    m_EntityMap[entity.GetUUID()] = (entt::entity)entity;
    return entity;
}

Entity Scene::CreateEntityWithUUID(const std::string& name, UUID uuid)
{
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<TransformComponent>(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    entity.AddComponent<WorldTransformComponent>(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    entity.AddComponent<SceneTreeComponent>();
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<UUIDComponent>(uuid);
    m_EntityMap[uuid] = (entt::entity)entity;
    return entity;
}

void Scene::DestroyEntity(entt::entity entity) {
    if (entity == entt::null || !m_Registry.valid(entity)) return;

    if (HasComponent<SceneTreeComponent>(entity)) {
        auto& nodeComponent = GetComponent<SceneTreeComponent>(entity);
        if (nodeComponent.parent != entt::null && m_Registry.valid(nodeComponent.parent)) {
            auto& parentNode = GetComponent<SceneTreeComponent>(nodeComponent.parent);

            auto childIt = std::find(parentNode.children.begin(), parentNode.children.end(), entity);
            if (childIt != parentNode.children.end()) {
                parentNode.children.erase(childIt);
            }

            UUID entityUUID = GetComponent<UUIDComponent>(entity).uuid;
            auto uuidIt = std::find(parentNode.childrenUUIDs.begin(), parentNode.childrenUUIDs.end(), entityUUID);
            if (uuidIt != parentNode.childrenUUIDs.end()) {
                parentNode.childrenUUIDs.erase(uuidIt);
            }
        }
    }

    if (HasComponent<SceneTreeComponent>(entity)) {
        auto& nodeComponent = GetComponent<SceneTreeComponent>(entity);
        auto childrenCopy = nodeComponent.children; // Kopya oluştur
        for (auto child : childrenCopy) {
            DestroyEntity(child);
        }
    }

    if (HasComponent<MeshRendererComponent>(entity))
        GetComponent<MeshRendererComponent>(entity).Destroy();

    if (HasComponent<UUIDComponent>(entity)) {
        auto& uuidComponent = GetComponent<UUIDComponent>(entity);
        m_EntityMap.erase(uuidComponent.uuid);
    }

    m_Registry.destroy(entity);
    MaterialManager::ClearExpiredMaterials();
}


Entity Scene::CreatePointLight()
{
    Entity entity = CreateEntity("PointLight");
    AddComponent<PointLightComponent>(entity);
    return entity;
}

Entity Scene::CreateDirectionalLight()
{
    Entity entity = CreateEntity("DirectionalLight");
    GetComponent<TransformComponent>(entity).SetEulerRotation(glm::vec3(-60.0, -90.0, 0.0));
    AddComponent<DirectionalLightComponent>(entity);
    return entity;
}

Entity Scene::LoadAsset(const std::string& path)
{
    Entity rootEntity = CreateEntity("Model");

    std::vector<MeshData> meshDatas = AssetManager::LoadModel(TO_ABSOLUTE_PATH(path));

    if (meshDatas.size() == 1)
    {
        rootEntity.AddComponent<MeshRendererComponent>(meshDatas.back().vertices.data(), meshDatas.back().vertices.size() * sizeof(float),
            meshDatas.back().indices.data(), meshDatas.back().indices.size() * sizeof(unsigned int), meshDatas.back().indices.size());
        rootEntity.AddComponent<MeshComponent>(TO_ABSOLUTE_PATH(path), meshDatas.back().meshIndex);
        rootEntity.GetComponent<TagComponent>().tag = meshDatas.back().name;
        auto& materialComponent = rootEntity.AddComponent<MaterialComponent>(meshDatas.front().materialPath);

    }
    else
    {
        for (auto& meshData : meshDatas)
        {
            Entity meshEntity = CreateEntity("child");
            SetParent(meshEntity, rootEntity);
            meshEntity.AddComponent<MeshRendererComponent>(meshData.vertices.data(), meshData.vertices.size() * sizeof(float),
                meshData.indices.data(), meshData.indices.size() * sizeof(unsigned int), meshData.indices.size());
            meshEntity.AddComponent<MeshComponent>(TO_ABSOLUTE_PATH(path), meshData.meshIndex);
            meshEntity.GetComponent<TagComponent>().tag = meshData.name;
            auto& materialComponent = meshEntity.AddComponent<MaterialComponent>(meshData.materialPath);

        }
    }


    return rootEntity;

}

struct PrimitiveConfig {
    std::string name;
    Path meshPath;
};

Entity Scene::LoadPrimitive(const std::string& primitiveMeshName)
{
    // Define primitive configurations
    static const std::unordered_map<std::string, PrimitiveConfig> primitiveMap = {
        {"Cube", {"Cube", TO_ABSOLUTE_PATH("Assets\\Models\\Primitives\\Cube.fbx")}},
        {"Sphere", {"Sphere", TO_ABSOLUTE_PATH("Assets\\Models\\Primitives\\Sphere.fbx")}},
        {"Cylinder", {"Cylinder", TO_ABSOLUTE_PATH("Assets\\Models\\Primitives\\Cylinder.fbx")}},
        {"Cone", {"Cone", TO_ABSOLUTE_PATH("Assets\\Models\\Primitives\\Cone.fbx")}},
        {"Capsule", {"Capsule", TO_ABSOLUTE_PATH("Assets\\Models\\Primitives\\Capsule.fbx")}},
        {"Torus", {"Torus", TO_ABSOLUTE_PATH("Assets\\Models\\Primitives\\Torus.fbx")}}
    };

    // Check if the primitive exists
    auto it = primitiveMap.find(primitiveMeshName);
    if (it == primitiveMap.end()) {
        // Handle invalid primitive name (e.g., log error or return empty entity)
        return Entity{};
    }

    const PrimitiveConfig& config = it->second;

    // Create entity and load mesh
    Entity rootEntity = CreateEntity(config.name);
    MeshData meshData = AssetManager::LoadMesh(config.meshPath, 0);

    // Add components
    rootEntity.AddComponent<MeshRendererComponent>(
        meshData.vertices.data(),
        meshData.vertices.size() * sizeof(float),
        meshData.indices.data(),
        meshData.indices.size() * sizeof(unsigned int),
        meshData.indices.size()
    );
    rootEntity.AddComponent<MeshComponent>(config.meshPath, meshData.meshIndex);
    rootEntity.GetComponent<TagComponent>().tag = meshData.name;
    rootEntity.AddComponent<MaterialComponent>(meshData.materialPath);

    return rootEntity;
}


void Scene::Update() {
    UpdateHierarchy();
}

entt::registry& Scene::GetRegistry()
{
    return m_Registry;
}

void Scene::UpdateHierarchy() {
    auto view = m_Registry.view<SceneTreeComponent, TransformComponent, WorldTransformComponent>();

    std::vector<entt::entity> rootEntities;
    for (auto entity : view) {
        auto& node = m_Registry.get<SceneTreeComponent>(entity);
        if (node.parent == entt::null || !m_Registry.valid(node.parent)) {
            rootEntities.push_back(entity);
        }
    }

    for (auto root : rootEntities) {
        UpdateNodeRecursive(root, { 0, 0, 0 }, glm::quat(1, 0, 0, 0), { 1, 1, 1 });
    }
}

void Scene::UpdateNodeRecursive(entt::entity entity,
    const glm::vec3& parentPos,
    const glm::quat& parentRot,
    const glm::vec3& parentScale)
{
    auto& transform = m_Registry.get<TransformComponent>(entity);
    auto& worldTransform = m_Registry.get<WorldTransformComponent>(entity);

    // World transform hesaplama
    glm::vec3 worldPos = parentPos + parentRot * (transform.position * parentScale);
    glm::quat worldRot = parentRot * transform.rotation;
    glm::vec3 worldScale = parentScale * transform.scale;

    worldTransform.position = worldPos;
    worldTransform.rotation = worldRot;
    worldTransform.scale = worldScale;

    // Çocukları güncelle
    auto& node = m_Registry.get<SceneTreeComponent>(entity);
    for (auto child : node.children) {
        UpdateNodeRecursive(child, worldPos, worldRot, worldScale);
    }
}

void Scene::SetParent(entt::entity child, entt::entity parent) {

    if (parent != entt::null && !HasComponent<SceneTreeComponent>(parent))
        AddComponent<SceneTreeComponent>(parent);

    auto& childNode = GetComponent<SceneTreeComponent>(child);
    UUID childUUID = GetComponent<UUIDComponent>(child).uuid;

    glm::mat4 childWorldMatrix;
    if (HasComponent<WorldTransformComponent>(child)) {
        auto& childWorldTransform = GetComponent<WorldTransformComponent>(child);
        childWorldMatrix = childWorldTransform.GetMatrix();
    }
    else {
        auto& childTransform = GetComponent<TransformComponent>(child);
        childWorldMatrix = childTransform.GetMatrix();
    }

    if (childNode.parent != entt::null) {
        auto& oldParentNode = GetComponent<SceneTreeComponent>(childNode.parent);
        oldParentNode.children.erase(
            std::remove(oldParentNode.children.begin(), oldParentNode.children.end(), child),
            oldParentNode.children.end()
        );
        oldParentNode.childrenUUIDs.erase(
            std::remove(oldParentNode.childrenUUIDs.begin(), oldParentNode.childrenUUIDs.end(), childUUID),
            oldParentNode.childrenUUIDs.end()
        );
    }

    childNode.parent = parent;
    childNode.parentUUID = parent != entt::null ? GetComponent<UUIDComponent>(parent).uuid : 0;

    if (parent != entt::null) {
        auto& parentNode = GetComponent<SceneTreeComponent>(parent);
        parentNode.children.push_back(child);
        if (std::find(parentNode.childrenUUIDs.begin(),
            parentNode.childrenUUIDs.end(), childUUID)
            == parentNode.childrenUUIDs.end())
        {
            parentNode.childrenUUIDs.push_back(childUUID);
        }

        auto& parentWorldTransform = GetComponent<WorldTransformComponent>(parent);
        glm::mat4 parentWorldMatrix = parentWorldTransform.GetMatrix();
        glm::mat4 parentInverseMatrix = glm::inverse(parentWorldMatrix);

        glm::mat4 newLocalMatrix = parentInverseMatrix * childWorldMatrix;

        glm::vec3 scale, translation, skew;
        glm::quat rotation;
        glm::vec4 perspective;
        glm::decompose(newLocalMatrix, scale, rotation, translation, skew, perspective);

        auto& childTransform = GetComponent<TransformComponent>(child);
        childTransform.position = translation;
        childTransform.rotation = rotation; // Direkt quaternion ata
        childTransform.scale = scale;
    }
    else {
        glm::vec3 scale, rotation, position;
        DecomposeToEulerAngles(childWorldMatrix, scale, rotation, position);

        auto& childTransform = GetComponent<TransformComponent>(child);
        childTransform.scale = scale;
        childTransform.rotation = rotation;
        childTransform.position = position;
    }
}

void Scene::RemoveParent(entt::entity child) {
    if (!HasComponent<SceneTreeComponent>(child))
        return;

    auto& childNode = GetComponent<SceneTreeComponent>(child);
    UUID childUUID = GetComponent<UUIDComponent>(child).uuid;

    // 1) World matrisi al
    glm::mat4 childWorldMatrix(1.0f);
    if (HasComponent<WorldTransformComponent>(child)) {
        childWorldMatrix = GetComponent<WorldTransformComponent>(child).GetMatrix();
    }
    else {
        childWorldMatrix = GetComponent<TransformComponent>(child).GetMatrix();
    }

    // 2) Parent ilişkisini kaldır
    if (childNode.parent != entt::null) {
        auto& parentNode = GetComponent<SceneTreeComponent>(childNode.parent);
        parentNode.children.erase(
            std::remove(parentNode.children.begin(), parentNode.children.end(), child),
            parentNode.children.end()
        );
        parentNode.childrenUUIDs.erase(
            std::remove(parentNode.childrenUUIDs.begin(), parentNode.childrenUUIDs.end(), childUUID),
            parentNode.childrenUUIDs.end()
        );
    }
    childNode.parent = entt::null;
    childNode.parentUUID = 0;

    // 3) World pozisyon/ölçek/Euler elde et (istersen sadece pozisyon + ölçek dekompoze edebilirsin)
    glm::vec3 newScale, newEuler, newPosition;
    DecomposeToEulerAngles(childWorldMatrix, newScale, newEuler, newPosition);

    // 4) World quaternion’ı al
    glm::quat worldQuat = glm::quat_cast(childWorldMatrix);

    // 5) TransformComponent’e ata
    auto& tc = GetComponent<TransformComponent>(child);
    tc.position = newPosition;
    tc.scale = newScale;
    tc.SetRotation(worldQuat);  // hem rotationı hem de rotation_degrees’ı günceller
}

std::vector<PointLightData> Scene::CollectPointLights() noexcept
{
    std::vector<PointLightData> lights;
    auto view = GetRegistry().view<PointLightComponent, WorldTransformComponent>();

    for (auto entity : view)
    {
        auto& light = GetComponent<PointLightComponent>(entity);
        auto& transform = GetComponent<WorldTransformComponent>(entity);

        PointLightData data;
        data.position = glm::vec3(transform.GetMatrix()[3]);
        data.color = light.color;
        data.intensity = light.intensity;
        data.radius = light.radius;

        lights.push_back(data);
    }

    return lights;
}

std::optional<DirectionalLightData> Scene::CollectDirectionalLight() noexcept
{
    auto view = GetRegistry().view<DirectionalLightComponent, WorldTransformComponent>();

    for (auto entity : view)
    {
        auto& light = GetComponent<DirectionalLightComponent>(entity);
        auto& transform = GetComponent<WorldTransformComponent>(entity);

        DirectionalLightData data;
        data.direction = transform.GetForward();
        data.color = light.color;
        data.intensity = light.intensity;
        data.castShadows = light.castShadows;

        return data;
    }

    return std::nullopt;
}