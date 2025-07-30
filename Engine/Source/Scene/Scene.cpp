#include "rvelapch.h"
#include "Scene.h"
#include "Entity.h"
#include "Core/Time.h"
#include "Core/Log.h"
#include "Utils/AssetManager.h"
#include "Utils/Serializer.h"
#include "Utils/MaterialManager.h"
#include "Utils/ProjectManager.h"
#include "EntityUUID.h"


Scene::Scene() : m_Registry() { LoadPrimitive("Cube"); CreateDirectionalLight(); }

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<TransformComponent>(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    entity.AddComponent<SceneTreeComponent>();
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<UUIDComponent>(EntityUUIDGenerator::Generate());
    m_EntityMap[entity.GetUUID()] = (entt::entity)entity;
    return entity;
}

Entity Scene::CreateEntityWithUUID(const std::string& name, EntityUUID uuid) {
    Entity entity(m_Registry.create(), this);
    entity.AddComponent<TransformComponent>(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    entity.AddComponent<SceneTreeComponent>();
    entity.AddComponent<TagComponent>(name);
    entity.AddComponent<UUIDComponent>(uuid);
    m_EntityMap[uuid] = (entt::entity)entity;
    return entity;
}

void Scene::DestroyEntity(entt::entity entity) {
    if (entity == entt::null || !m_Registry.valid(entity)) return;
    if (HasComponent<SceneTreeComponent>(entity)) {
        auto& node = GetComponent<SceneTreeComponent>(entity);
        if (node.parent != entt::null && m_Registry.valid(node.parent)) {
            auto& parentNode = GetComponent<SceneTreeComponent>(node.parent);
            auto it = std::find(parentNode.children.begin(), parentNode.children.end(), entity);
            if (it != parentNode.children.end()) parentNode.children.erase(it);
            auto uuidIt = std::find(parentNode.childrenUUIDs.begin(), parentNode.childrenUUIDs.end(), GetComponent<UUIDComponent>(entity).uuid);
            if (uuidIt != parentNode.childrenUUIDs.end()) parentNode.childrenUUIDs.erase(uuidIt);
        }
    }
    if (HasComponent<SceneTreeComponent>(entity)) {
        auto& node = GetComponent<SceneTreeComponent>(entity);
        auto childrenCopy = node.children;
        for (auto child : childrenCopy) DestroyEntity(child);
    }
    if (HasComponent<MeshRendererComponent>(entity)) GetComponent<MeshRendererComponent>(entity).Destroy();
    if (HasComponent<UUIDComponent>(entity)) m_EntityMap.erase(GetComponent<UUIDComponent>(entity).uuid);
    m_Registry.destroy(entity);
}

Entity Scene::CreatePointLight() {
    Entity entity = CreateEntity("PointLight");
    AddComponent<PointLightComponent>(entity);
    return entity;
}

Entity Scene::CreateDirectionalLight() {
    Entity entity = CreateEntity("DirectionalLight");
    GetComponent<TransformComponent>(entity).SetEulerRotation(glm::vec3(-60.0f, -90.0f, 0.0f));
    AddComponent<DirectionalLightComponent>(entity);
    return entity;
}

Entity Scene::LoadAsset(const std::string& path) {
    Entity root = CreateEntity("Model");
    std::vector<MeshData> meshes = AssetManager::LoadModel(VRT_PATH(path));
    if (meshes.size() == 1) {
        auto& m = meshes.back();
        root.AddComponent<MeshRendererComponent>(
            m.vertices.data(), 
            m.vertices.size() * sizeof(float), 
            m.indices.data(), 
            m.indices.size() * sizeof(unsigned int), 
            m.indices.size(),
            m.localAABB);
        root.AddComponent<MeshComponent>(VRT_PATH(path), m.meshIndex,m);
        root.GetComponent<TagComponent>().tag = m.name;
        root.AddComponent<MaterialComponent>(AssetUUID::FromString("ee3dde12-6263-4f11-bb1d-812b3e196ab7"));
    }
    else {
        for (auto& m : meshes) {
            Entity child = CreateEntity("child");
            SetParent(child, root);
            child.AddComponent<MeshRendererComponent>(
                m.vertices.data(), 
                m.vertices.size() * sizeof(float), 
                m.indices.data(), 
                m.indices.size() * sizeof(unsigned int), 
                m.indices.size(),
                m.localAABB);
            child.AddComponent<MeshComponent>(VRT_PATH(path), m.meshIndex,m);
            child.GetComponent<TagComponent>().tag = m.name;
            child.AddComponent<MaterialComponent>(AssetUUID::FromString("ee3dde12-6263-4f11-bb1d-812b3e196ab7"));
        }
    }
    return root;
}

struct PrimitiveConfig { std::string name; Path meshPath; };
Entity Scene::LoadPrimitive(const std::string& primitiveMeshName) {
    static const std::unordered_map<std::string, PrimitiveConfig> map = {
        {"Cube", {"Cube", VRT_PATH("Assets\\Models\\Primitives\\Cube.fbx")}},
        {"Sphere", {"Sphere", VRT_PATH("Assets\\Models\\Primitives\\Sphere.fbx")}},
        {"Cylinder", {"Cylinder", VRT_PATH("Assets\\Models\\Primitives\\Cylinder.fbx")}},
        {"Cone", {"Cone", VRT_PATH("Assets\\Models\\Primitives\\Cone.fbx")}},
        {"Capsule", {"Capsule", VRT_PATH("Assets\\Models\\Primitives\\Capsule.fbx")}},
        {"Torus", {"Torus", VRT_PATH("Assets\\Models\\Primitives\\Torus.fbx")}}
    };
    auto it = map.find(primitiveMeshName);
    if (it == map.end()) return Entity{};
    const auto& cfg = it->second;
    Entity root = CreateEntity(cfg.name);
    auto m = AssetManager::LoadMesh(cfg.meshPath, 0);
    root.AddComponent<MeshRendererComponent>(m.vertices.data(), m.vertices.size() * sizeof(float), m.indices.data(), m.indices.size() * sizeof(unsigned int), m.indices.size(),m.localAABB);
    root.AddComponent<MeshComponent>(cfg.meshPath, m.meshIndex,m);
    root.GetComponent<TagComponent>().tag = m.name;
    AssetUUID id = AssetUUID::FromString("ee3dde12-6263-4f11-bb1d-812b3e196ab7");
    root.AddComponent<MaterialComponent>(id);
    return root;
}

void Scene::Update() { UpdateHierarchy(); }
entt::registry& Scene::GetRegistry() { return m_Registry; }

void Scene::UpdateHierarchy() {
    auto view = m_Registry.view<SceneTreeComponent, TransformComponent>();
    std::vector<entt::entity> roots;
    for (auto e : view) {
        auto& node = GetComponent<SceneTreeComponent>(e);
        if (node.parent == entt::null || !m_Registry.valid(node.parent))
            roots.push_back(e);
    }
    for (entt::entity root : roots)
        if(GetComponent<TransformComponent>(root).IsDirty())
            UpdateNodeRecursive(root, glm::mat4(1.0f));
}

void Scene::UpdateNodeRecursive(entt::entity e, const glm::mat4& parentWorld) {
    auto& t = GetComponent<TransformComponent>(e);

    glm::mat4 localMat = t.GetLocalMatrix();
    glm::mat4 worldMat = parentWorld * localMat;

    glm::vec3 scale, skew, translation;
    glm::quat rotation;
    glm::vec4 perspective;
    glm::decompose(worldMat, scale, rotation, translation, skew, perspective);

    t.SetWorldTransform(translation, rotation, scale);

    //Update AABBs
    if (HasComponent<MeshRendererComponent>(e))
    {
        auto& c = GetComponent<MeshRendererComponent>(e);
        auto& localAABB = c.localAABB;

        

        glm::vec3 localMin = localAABB.min;
        glm::vec3 localMax = localAABB.max;

        glm::vec3 corners[8] = {
            {localMin.x, localMin.y, localMin.z},
            {localMax.x, localMin.y, localMin.z},
            {localMin.x, localMax.y, localMin.z},
            {localMax.x, localMax.y, localMin.z},
            {localMin.x, localMin.y, localMax.z},
            {localMax.x, localMin.y, localMax.z},
            {localMin.x, localMax.y, localMax.z},
            {localMax.x, localMax.y, localMax.z},
        };

        glm::vec3 worldMin(FLT_MAX);
        glm::vec3 worldMax(-FLT_MAX);

        for (int i = 0; i < 8; ++i) {
            glm::vec4 worldPos = t.GetWorldMatrix() * glm::vec4(corners[i], 1.0f);
            glm::vec3 p = glm::vec3(worldPos);

            worldMin = glm::min(worldMin, p);
            worldMax = glm::max(worldMax, p);
        }

        BoundingBox worldAABB(worldMin, worldMax);
        c.worldAABB = worldAABB;
    }

    t.ClearDirty();

    auto& node = GetComponent<SceneTreeComponent>(e);
    for (auto c : node.children)
        UpdateNodeRecursive(c, worldMat);
}

void Scene::SetParent(entt::entity child, entt::entity parent) {
    if (parent != entt::null && !HasComponent<SceneTreeComponent>(parent)) AddComponent<SceneTreeComponent>(parent);
    auto& node = GetComponent<SceneTreeComponent>(child);
    EntityUUID id = GetComponent<UUIDComponent>(child).uuid;
    glm::mat4 childWorld = GetComponent<TransformComponent>(child).GetWorldMatrix();
    if (node.parent != entt::null) {
        auto& old = GetComponent<SceneTreeComponent>(node.parent);
        old.children.erase(std::remove(old.children.begin(), old.children.end(), child), old.children.end());
        old.childrenUUIDs.erase(std::remove(old.childrenUUIDs.begin(), old.childrenUUIDs.end(), id), old.childrenUUIDs.end());
    }
    node.parent = parent;
    node.parentUUID = parent != entt::null ? GetComponent<UUIDComponent>(parent).uuid : 0;
    if (parent != entt::null) {
        auto& pnode = GetComponent<SceneTreeComponent>(parent);
        pnode.children.push_back(child);
        if (std::find(pnode.childrenUUIDs.begin(), pnode.childrenUUIDs.end(), id) == pnode.childrenUUIDs.end()) pnode.childrenUUIDs.push_back(id);
        glm::mat4 pWorld = GetComponent<TransformComponent>(parent).GetWorldMatrix();
        glm::mat4 inv = glm::inverse(pWorld);
        glm::mat4 localM = inv * childWorld;
        glm::vec3 scale, translation, skew;
        glm::quat rotation;
        glm::vec4 persp;
        glm::decompose(localM, scale, rotation, translation, skew, persp);
        auto& tc = GetComponent<TransformComponent>(child);
        tc.SetPosition(translation);
        tc.SetRotation(rotation);
        tc.SetScale(scale);
    }
    else {
        glm::vec3 scale, euler, pos;
        DecomposeToEulerAngles(childWorld, scale, euler, pos);
        auto& tc = GetComponent<TransformComponent>(child);
        tc.SetPosition(pos);
        tc.SetRotation(glm::quat_cast(childWorld));
        tc.SetScale(scale);
    }
}

void Scene::RemoveParent(entt::entity child) {
    if (!HasComponent<SceneTreeComponent>(child)) return;
    auto& node = GetComponent<SceneTreeComponent>(child);
    EntityUUID id = GetComponent<UUIDComponent>(child).uuid;
    glm::mat4 worldM = GetComponent<TransformComponent>(child).GetWorldMatrix();
    if (node.parent != entt::null) {
        auto& pnode = GetComponent<SceneTreeComponent>(node.parent);
        pnode.children.erase(std::remove(pnode.children.begin(), pnode.children.end(), child), pnode.children.end());
        pnode.childrenUUIDs.erase(std::remove(pnode.childrenUUIDs.begin(), pnode.childrenUUIDs.end(), id), pnode.childrenUUIDs.end());
    }
    node.parent = entt::null;
    node.parentUUID = 0;
    glm::vec3 newScale, newEuler, newPos;
    DecomposeToEulerAngles(worldM, newScale, newEuler, newPos);
    glm::quat worldQuat = glm::quat_cast(worldM);
    auto& tc = GetComponent<TransformComponent>(child);
    tc.SetPosition(newPos);
    tc.SetScale(newScale);
    tc.SetRotation(worldQuat);
}

std::vector<PointLightData> Scene::CollectPointLights() noexcept {
    std::vector<PointLightData> lights;
    auto view = GetRegistry().view<PointLightComponent, TransformComponent>();
    for (auto e : view) {
        auto& light = GetComponent<PointLightComponent>(e);
        auto& t = GetComponent<TransformComponent>(e);
        PointLightData data;
        data.position = t.GetWorldPosition();
        data.color = light.color;
        data.intensity = light.intensity;
        data.radius = light.radius;
        data.falloff = light.falloff;
        data.castShadows = light.castShadows;
        data.shadowIndex = light.shadowIndex;
        data.shadowBias = light.shadowBias;
        data.reverseCullFace = light.reverseCullFace;
        data.blurRadius = light.blurRadius;
        lights.push_back(data);
    }
    return lights;
}

std::optional<DirectionalLightData> Scene::CollectDirectionalLight() noexcept {
    auto view = GetRegistry().view<DirectionalLightComponent, TransformComponent>();
    for (auto e : view) {
        auto& light = GetComponent<DirectionalLightComponent>(e);
        auto& t = GetComponent<TransformComponent>(e);
        DirectionalLightData data;
        data.direction = t.GetForward();
        data.color = light.color;
        data.intensity = light.intensity;
        data.shadowBias = light.shadowBias;
        data.castShadows = light.castShadows;
        data.reverseCullFace = light.reverseCullFace;
        data.blurRadius = light.blurRadius;

        return data;
    }
    return std::nullopt;
}
