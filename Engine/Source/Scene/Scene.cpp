#include "rvelapch.h"
#include "Scene.h"
#include "SceneSerializer.h"
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
m_AudioSystem(*this),
m_ParticleSystem(*this),
m_AnimationSystem(*this)
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

#include "Input/Input.h"

void Scene::OnStart()
{
    m_PhysicsSystem.OnStart();
    m_ScriptSystem.OnStart();
    m_AudioSystem.OnStart();
    m_AnimationSystem.OnStart();
    m_UISystem.Init();
}

void Scene::OnUpdate(float dt)
{
    m_ScriptSystem.OnUpdate(dt);
    
    glm::vec2 mousePos = Input::GetViewportMousePosition(1920.0f, 1080.0f);
    bool pressed = Input::IsMouseButtonJustPressed(MouseCode::ButtonLeft);
    bool held = Input::IsMouseButtonPressed(MouseCode::ButtonLeft);
    bool released = Input::IsMouseButtonJustReleased(MouseCode::ButtonLeft);
    m_UISystem.Update(this, dt, glm::vec2(1920.0f, 1080.0f), mousePos, pressed, held, released);

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
    m_UISystem.Shutdown();
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
    entity.GetComponent<UUIDComponent>().uuid = EntityUUIDGenerator::GeneratePersistent();
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
    m_isUpdating = true;

    if (m_State == SceneState::PLAY)
    {
        OnUpdate(Time::GetDeltaTime());
    }

    m_TransformSystem.Update();
    m_PhysicsSystem.Update();
    m_CameraSystem.Update();
    m_AudioSystem.Update();
    m_ParticleSystem.Update(Time::GetDeltaTime());
    m_AnimationSystem.Update();

    m_isUpdating = false;
    FlushDestroyQueue();
}
void Scene::FixedUpdate()
{
    m_isUpdating = true;

    if (m_State == SceneState::PLAY)
    {
        OnFixedUpdate(Time::GetFixedDeltaTime());
    }

    m_isUpdating = false;
    FlushDestroyQueue();
}

void Scene::LateUpdate()
{
    m_isUpdating = true;

    if (m_State == SceneState::PLAY)
    {
        OnLateUpdate(Time::GetDeltaTime());
    }

    m_isUpdating = false;
    FlushDestroyQueue();
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

bool Scene::IsEntitySelfActive(entt::entity entity)
{
    if (entity == entt::null || !m_Registry.valid(entity))
        return false;

    if (auto* tag = m_Registry.try_get<TagComponent>(entity))
        return tag->isActive;

    return true;
}

bool Scene::IsEntityActive(entt::entity entity)
{
    if (entity == entt::null || !m_Registry.valid(entity))
        return false;

    entt::entity curr = entity;
    while (curr != entt::null && m_Registry.valid(curr))
    {
        if (curr == m_RootEntity)
            break;

        if (auto* tag = m_Registry.try_get<TagComponent>(curr))
        {
            if (!tag->isActive)
                return false;
        }

        if (!HasComponent<SceneTreeComponent>(curr))
            break;

        curr = GetComponent<SceneTreeComponent>(curr).parent;
    }

    return true;
}

void Scene::SetEntityActive(entt::entity entity, bool active)
{
    if (entity == entt::null || !m_Registry.valid(entity))
        return;

    if (auto* tag = m_Registry.try_get<TagComponent>(entity))
    {
        tag->isActive = active;
    }
}


void Scene::SetParent(entt::entity child, entt::entity parent)
{
    m_TransformSystem.SetParent(child, parent);
}

void Scene::SetParentKeepLocal(entt::entity child, entt::entity parent)
{
    m_TransformSystem.SetParentKeepLocal(child, parent);
}

void Scene::RemoveParent(entt::entity child)
{
    m_TransformSystem.RemoveParent(child);
}

Entity Scene::Instantiate(const AssetUUID& prefabUUID)
{
    return Instantiate(prefabUUID, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), entt::null);
}

Entity Scene::Instantiate(const AssetUUID& prefabUUID, const glm::vec3& position, const glm::quat& rotation, entt::entity parent)
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

    if (!prefabJson.contains("Entities") || !prefabJson["Entities"].is_array() || prefabJson["Entities"].empty())
    {
        LOG_ERROR("[Scene::Instantiate] No Entities in prefab: {}", prefabUUID.ToString());
        return Entity{};
    }

    std::unordered_map<EntityUUID, entt::entity> oldToNewEntity;
    Entity rootEntity;

    for (auto& entityJson : prefabJson["Entities"])
    {
        Entity e = CreateEntityRaw();
        entt::entity handle = e.GetHandle();

        EntityUUID oldUUID = entityJson.contains("UUID") ? entityJson["UUID"].get<EntityUUID>() : 0;
        EntityUUID newUUID = EntityUUIDGenerator::GeneratePersistent();
        e.GetComponent<UUIDComponent>().uuid = newUUID;
        m_EntityMap[newUUID] = handle;

        if (oldUUID != 0)
            oldToNewEntity[oldUUID] = handle;

        SceneSerializer::DeserializeEntityComponents(*this, handle, entityJson);

        if (!e.HasComponent<TagComponent>())
            e.AddComponent<TagComponent>("Entity");

        if (entityJson.contains("_isRoot") && entityJson["_isRoot"] == true)
            rootEntity = e;
    }

    entt::entity defaultParent = (parent != entt::null && m_Registry.valid(parent)) ? parent : m_RootEntity;

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
            SetParentKeepLocal(handle, defaultParent);

            if (rootEntity.GetHandle() == entt::null)
                rootEntity = Entity(handle, this);
        }
    }

    if (rootEntity.GetHandle() != entt::null)
    {
        auto& tc = GetComponent<TransformComponent>(rootEntity.GetHandle());
        tc.SetPosition(position);
        tc.SetRotation(rotation);
        tc.SetDirty();

        AddComponent<PrefabComponent>(rootEntity.GetHandle(), prefabUUID);
    }

    m_TransformSystem.Update();
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

void Scene::MoveChildOrder(entt::entity source, entt::entity target, bool insertBefore)
{
    if (source == entt::null || target == entt::null || source == target) return;
    if (!m_Registry.valid(source) || !m_Registry.valid(target)) return;

    auto& sourceTree = GetComponent<SceneTreeComponent>(source);
    auto& targetTree = GetComponent<SceneTreeComponent>(target);

    entt::entity targetParent = targetTree.parent != entt::null ? targetTree.parent : m_RootEntity;
    if (sourceTree.parent != targetParent)
    {
        SetParent(source, targetParent);
    }

    auto& parentTree = GetComponent<SceneTreeComponent>(targetParent);
    auto& children = parentTree.children;
    auto& childrenUUIDs = parentTree.childrenUUIDs;

    EntityUUID sourceUUID = GetComponent<UUIDComponent>(source).uuid;

    std::erase(children, source);
    std::erase(childrenUUIDs, sourceUUID);

    auto it = std::find(children.begin(), children.end(), target);
    if (it != children.end())
    {
        size_t index = std::distance(children.begin(), it);
        if (!insertBefore)
        {
            index++;
        }

        if (index >= children.size())
        {
            children.push_back(source);
            childrenUUIDs.push_back(sourceUUID);
        }
        else
        {
            children.insert(children.begin() + index, source);
            childrenUUIDs.insert(childrenUUIDs.begin() + index, sourceUUID);
        }
    }
    else
    {
        children.push_back(source);
        childrenUUIDs.push_back(sourceUUID);
    }
}

Entity Scene::DuplicateEntity(entt::entity entityHandle)
{
    return SceneSerializer::CloneEntity(*this, entityHandle);
}

Entity Scene::DuplicateEntity(Entity entity)
{
    return DuplicateEntity(entity.GetHandle());
}