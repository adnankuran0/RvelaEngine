#include "rvelapch.h"
#include "Scene.h"
#include "Entity.h"
#include "Core/Time.h"
#include "Core/Log.h"
#include "Utils/Serializer.h"
#include "Utils/ProjectManager.h"
#include "EntityUUID.h"
#include "Asset/Types/PrefabAsset.h"
#include "Asset/AssetManager.h"
#include <glm/gtx/matrix_decompose.hpp>

using namespace rv;

Scene::Scene(const std::string& sceneName) : m_Registry() , 
m_ScriptSystem(*this), 
m_CameraSystem(*this), 
m_LightSystem(*this),
m_TransformSystem(*this),
m_PhysicsSystem(*this),
m_AudioSystem(*this)
{
    m_ScenePath = "";
    m_SceneName = sceneName;
    m_RootEntity = m_Registry.create();
    Entity entity(m_RootEntity, this);
    entity.AddComponent<TransformComponent>(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    entity.AddComponent<SceneTreeComponent>();
    entity.AddComponent<TagComponent>(sceneName);
    entity.AddComponent<UUIDComponent>(EntityUUIDGenerator::Generate());
    m_EntityMap[entity.GetUUID()] = (entt::entity)m_RootEntity;

}

Scene::~Scene()
{
    m_Registry.clear();
}

void Scene::SetState(SceneState newState)
{
    if (newState == m_State) return;
    if (m_State == SceneState::EDIT && newState == SceneState::PLAY)
        OnStart();
    if (m_State == SceneState::PLAY && newState == SceneState::EDIT)
        OnStop();
    m_State = newState;
}

void Scene::OnStart()
{
    m_PhysicsSystem.OnStart();
    m_ScriptSystem.OnStart();
}

void Scene::OnUpdate(float dt)
{
    m_ScriptSystem.OnUpdate(dt);
    JPH::BodyManager::DrawSettings settings;
    settings.mDrawBoundingBox = true;
    settings.mDrawShapeWireframe = true;
}

void rv::Scene::OnFixedUpdate(float dt)
{
    m_ScriptSystem.OnFixedUpdate(dt);
    m_PhysicsSystem.Step(dt);
}

void rv::Scene::OnLateUpdate(float dt)
{
    m_ScriptSystem.OnLateUpdate(dt);
}

void Scene::OnStop()
{
    m_ScriptSystem.OnStop();
}

Entity Scene::CreateEntityRaw()
{
    Entity e(m_Registry.create(), this);
    e.AddComponent<TransformComponent>();
    e.AddComponent<SceneTreeComponent>();
    e.AddComponent<UUIDComponent>();
    return e;
}

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity = CreateEntityRaw();
    entity.GetComponent<UUIDComponent>().uuid = EntityUUIDGenerator::Generate();
    entity.AddComponent<TagComponent>(name);
    SetParent(entity, m_RootEntity);
    m_EntityMap[entity.GetUUID()] = (entt::entity)entity;
    return entity;
}

Entity Scene::CreateEntityWithUUID(const std::string& name, EntityUUID uuid) {
    Entity entity = CreateEntityRaw();
    entity.GetComponent<UUIDComponent>().uuid = uuid;
    entity.AddComponent<TagComponent>(name);
    SetParent(entity, m_RootEntity);
    m_EntityMap[entity.GetUUID()] = (entt::entity)entity;
    return entity;
}

void Scene::DestroyEntity(entt::entity entity) {
    if (entity == entt::null || !m_Registry.valid(entity)) return;

    auto& node = GetComponent<SceneTreeComponent>(entity);
    if (node.parent != entt::null && m_Registry.valid(node.parent)) {
        auto& parentNode = GetComponent<SceneTreeComponent>(node.parent);
        auto it = std::find(parentNode.children.begin(), parentNode.children.end(), entity);
        if (it != parentNode.children.end()) parentNode.children.erase(it);
        auto uuidIt = std::find(parentNode.childrenUUIDs.begin(), parentNode.childrenUUIDs.end(), GetComponent<UUIDComponent>(entity).uuid);
        if (uuidIt != parentNode.childrenUUIDs.end()) parentNode.childrenUUIDs.erase(uuidIt);
    }

    auto childrenCopy = node.children;
    for (auto child : childrenCopy) 
        DestroyEntity(child);

    if (HasComponent<MeshRendererComponent>(entity)) GetComponent<MeshRendererComponent>(entity).Destroy();
    m_EntityMap.erase(GetComponent<UUIDComponent>(entity).uuid);
    m_Registry.destroy(entity);
}

void Scene::DestroyEntity(Entity& entity) {
    DestroyEntity(entity.GetHandle());
}

void Scene::QueueDestroyEntity(Entity& entity)
{
    QueueDestroyEntity(entity.GetHandle());
}

void Scene::QueueDestroyEntity(entt::entity entity)
{
    if (!m_isUpdating)
        DestroyEntity(entity);
    else
        m_pendingDestroys.push_back(entity);
}

void Scene::Update() 
{                

    if (m_State == SceneState::PLAY)
    {
        m_isUpdating = true;
        OnUpdate(Time::GetDeltaTime());
        m_isUpdating = false;
        FlushDestroyQueue();
    }
    m_TransformSystem.Update();
    m_PhysicsSystem.Update(); 
    m_CameraSystem.Update();
    m_AudioSystem.Update();
}
void Scene::FixedUpdate() 
{                
    if (m_State == SceneState::PLAY)
    {
        m_isUpdating = true;
        OnFixedUpdate(Time::GetFixedDeltaTime());
        m_isUpdating = false;
        FlushDestroyQueue(); 
    }
}
void Scene::LateUpdate() 
{              
    if (m_State == SceneState::PLAY)
    {
        m_isUpdating = true;
        OnLateUpdate(Time::GetDeltaTime());
        m_isUpdating = false;
        FlushDestroyQueue(); 
    }
}

entt::registry& Scene::GetRegistry() { return m_Registry; }

Entity rv::Scene::GetEntityByName(const std::string& name)
{
    auto view = m_Registry.view<TagComponent>();

    for (auto entityHandle : view) {
        Entity e(entityHandle, this);
        if (e.GetName() == name)
            return e;
    }

    return Entity{};
}



void Scene::SetParent(entt::entity child, entt::entity parent)
{
    if (child == entt::null || !m_Registry.valid(child)) return;

    if (child == parent) return;

    if (!HasComponent<SceneTreeComponent>(child))
        AddComponent<SceneTreeComponent>(child);

    auto& childNode = GetComponent<SceneTreeComponent>(child);

    if (childNode.parent == parent) return;

    if (parent != entt::null)
    {
        entt::entity curr = parent;
        while (curr != entt::null && m_Registry.valid(curr))
        {
            if (curr == child)
            {
                return;
            }
            if (!HasComponent<SceneTreeComponent>(curr))
                break;
            curr = GetComponent<SceneTreeComponent>(curr).parent;
        }
    }

    if (parent != entt::null && !HasComponent<SceneTreeComponent>(parent))
        AddComponent<SceneTreeComponent>(parent);

    EntityUUID childUUID = GetComponent<UUIDComponent>(child).uuid;
    glm::mat4 childWorldMatrix = GetComponent<TransformComponent>(child).GetWorldMatrix();

    if (childNode.parent != entt::null && m_Registry.valid(childNode.parent))
    {
        auto& oldParentNode = GetComponent<SceneTreeComponent>(childNode.parent);

        auto it = std::find(oldParentNode.children.begin(), oldParentNode.children.end(), child);
        if (it != oldParentNode.children.end())
            oldParentNode.children.erase(it);

        auto uuidIt = std::find(oldParentNode.childrenUUIDs.begin(),
            oldParentNode.childrenUUIDs.end(), childUUID);
        if (uuidIt != oldParentNode.childrenUUIDs.end())
            oldParentNode.childrenUUIDs.erase(uuidIt);

        GetComponent<TransformComponent>(childNode.parent).SetDirty();
    }

    childNode.parent = parent;
    childNode.parentUUID = (parent != entt::null) ? GetComponent<UUIDComponent>(parent).uuid : 0;

    if (parent != entt::null)
    {
        auto& parentNode = GetComponent<SceneTreeComponent>(parent);

        parentNode.children.push_back(child);

        if (std::find(parentNode.childrenUUIDs.begin(), parentNode.childrenUUIDs.end(), childUUID)
            == parentNode.childrenUUIDs.end())
        {
            parentNode.childrenUUIDs.push_back(childUUID);
        }

        glm::mat4 parentWorldMatrix = GetComponent<TransformComponent>(parent).GetWorldMatrix();
        glm::mat4 parentInverse = glm::inverse(parentWorldMatrix);

        glm::mat4 localMatrix = parentInverse * childWorldMatrix;

        glm::vec3 scale, translation, skew;
        glm::quat rotation;
        glm::vec4 perspective;
        glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);

        auto& childTransform = GetComponent<TransformComponent>(child);
        childTransform.SetPosition(translation);
        childTransform.SetRotation(rotation);
        childTransform.SetScale(scale);
        childTransform.SetDirty();

        GetComponent<TransformComponent>(parent).SetDirty();
    }
    else
    {
        glm::vec3 scale, euler, position;
        math::DecomposeToEulerAngles(childWorldMatrix, scale, euler, position);

        auto& childTransform = GetComponent<TransformComponent>(child);
        childTransform.SetPosition(position);
        childTransform.SetRotation(glm::quat(glm::radians(euler)));
        childTransform.SetScale(scale);
        childTransform.SetDirty();
    }
}
void Scene::SetParentKeepLocal(entt::entity child, entt::entity parent)
{
    auto& childTree = GetComponent<SceneTreeComponent>(child);

    if (childTree.parent != entt::null && m_Registry.valid(childTree.parent))
    {
        auto& oldParentTree = GetComponent<SceneTreeComponent>(childTree.parent);
        std::erase(oldParentTree.children, child);
    }

    childTree.parent = parent;

    if (parent != entt::null)
    {
        auto& parentTree = GetComponent<SceneTreeComponent>(parent);
        if (std::find(parentTree.children.begin(), parentTree.children.end(), child)
            == parentTree.children.end())
        {
            parentTree.children.push_back(child);
        }
    }
}

void Scene::RemoveParent(entt::entity child)
{
    SetParent(child, m_RootEntity);
}

Entity Scene::Instantiate(const AssetUUID& prefabUUID)
{
    Ref<PrefabAsset> prefab = AssetManager::Get().GetAsset<PrefabAsset>(prefabUUID);
    if (!prefab || !prefab->IsValid())
    {
        LOG_ERROR("[Scene::Instantiate] Prefab not found: {}", prefabUUID.ToString());
        return Entity{};
    }

    json prefabJson;
    try { prefabJson = json::parse(prefab->GetJSON()); }
    catch (const json::exception& e)
    {
        LOG_ERROR("[Scene::Instantiate] JSON parse error: {}", e.what());
        return Entity{};
    }

    if (!prefabJson.contains("Entities"))
    {
        LOG_ERROR("[Scene::Instantiate] No Entities in prefab: {}", prefabUUID.ToString());
        return Entity{};
    }

    std::unordered_map<EntityUUID, entt::entity> oldToNewEntity;
    Entity rootEntity;

    for (auto& entityJson : prefabJson["Entities"])
    {
        entt::entity handle = m_Registry.create();
        Entity e(handle, this);

        e.AddComponent<TransformComponent>();
        e.AddComponent<SceneTreeComponent>();
        e.AddComponent<TagComponent>();
        e.AddComponent<UUIDComponent>();

        EntityUUID oldUUID = entityJson.contains("UUID") ? entityJson["UUID"].get<EntityUUID>() : 0;

        EntityUUID newUUID = EntityUUIDGenerator::Generate();
        e.GetComponent<UUIDComponent>().uuid = newUUID;
        m_EntityMap[newUUID] = handle;

        if (oldUUID != 0)
            oldToNewEntity[oldUUID] = handle;

        if (entityJson.contains("Tag"))
            e.GetComponent<TagComponent>().Deserialize(entityJson["Tag"]);
        else
            e.AddComponent<TagComponent>("Entity");

        if (entityJson.contains("Transform"))
            e.GetComponent<TransformComponent>().Deserialize(entityJson["Transform"]);

        if (entityJson.contains("Material"))
            e.AddComponent<MaterialComponent>().Deserialize(entityJson["Material"]);

        if (entityJson.contains("Mesh"))
        {
            auto& comp = e.AddComponent<MeshComponent>();
            comp.Deserialize(entityJson["Mesh"]);
            e.AddComponent<MeshRendererComponent>(comp.GetMesh());
        }

        if (entityJson.contains("PointLight"))
            e.AddComponent<PointLightComponent>().Deserialize(entityJson["PointLight"]);

        if (entityJson.contains("DirectionalLight"))
            e.AddComponent<DirectionalLightComponent>().Deserialize(entityJson["DirectionalLight"]);

        if (entityJson.contains("CameraComponent"))
            e.AddComponent<CameraComponent>().Deserialize(entityJson["CameraComponent"]);

        if (entityJson.contains("RigidbodyComponent"))
            e.AddComponent<RigidbodyComponent>().Deserialize(entityJson["RigidbodyComponent"]);

        if (entityJson.contains("CharacterBodyComponent"))
            e.AddComponent<CharacterBodyComponent>().Deserialize(entityJson["CharacterBodyComponent"]);

        if (entityJson.contains("BoxColliderComponent"))
            e.AddComponent<BoxColliderComponent>().Deserialize(entityJson["BoxColliderComponent"]);

        if (entityJson.contains("SphereColliderComponent"))
            e.AddComponent<SphereColliderComponent>().Deserialize(entityJson["SphereColliderComponent"]);

        if (entityJson.contains("CapsuleColliderComponent"))
            e.AddComponent<CapsuleColliderComponent>().Deserialize(entityJson["CapsuleColliderComponent"]);

        if (entityJson.contains("CylinderColliderComponent"))
            e.AddComponent<CylinderColliderComponent>().Deserialize(entityJson["CylinderColliderComponent"]);

        if (entityJson.contains("MeshColliderComponent"))
            e.AddComponent<MeshColliderComponent>().Deserialize(entityJson["MeshColliderComponent"]);

        if (entityJson.contains("ConvexHullColliderComponent"))
            e.AddComponent<ConvexHullColliderComponent>().Deserialize(entityJson["ConvexHullColliderComponent"]);

        if (entityJson.contains("ScriptComponent"))
            e.AddComponent<ScriptComponent>().Deserialize(entityJson["ScriptComponent"]);

        if (entityJson.contains("ParentUUID"))
            e.GetComponent<SceneTreeComponent>().parentUUID = entityJson["ParentUUID"];

        if (entityJson.contains("_isRoot") && entityJson["_isRoot"] == true)
            rootEntity = e;
    }

    for (auto& [oldUUID, handle] : oldToNewEntity)
    {
        auto& tree = GetComponent<SceneTreeComponent>(handle);

        if (tree.parentUUID != 0 && oldToNewEntity.contains(tree.parentUUID))
        {
            entt::entity newParent = oldToNewEntity[tree.parentUUID];
            SetParentKeepLocal(handle, newParent);
        }
        else
        {
            SetParentKeepLocal(handle, m_RootEntity);

            if (rootEntity.GetHandle() == entt::null)
                rootEntity = Entity(handle, this);
        }
    }

    if (rootEntity.GetHandle() != entt::null)
    {
        auto& tc = GetComponent<TransformComponent>(rootEntity.GetHandle());
        tc.SetPosition(glm::vec3(0.0f));
        tc.SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

        AddComponent<PrefabComponent>(rootEntity.GetHandle(), prefabUUID);
    }

    return rootEntity;
}

unsigned int Scene::CountEntitiesRecursively(entt::entity& rootEntity)
{
    unsigned int count = 0;
    if (HasComponent<SceneTreeComponent>(rootEntity))
    {
        auto& children = GetComponent<SceneTreeComponent>(rootEntity).children;
        for (auto child : children)
            count += CountEntitiesRecursively(child);
    }

    return count;
}
