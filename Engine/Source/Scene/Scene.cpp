#include "rvelapch.h"
#include "Scene.h"
#include "Entity.h"
#include "Core/Time.h"
#include "Core/Log.h"
#include "Utils/Serializer.h"
#include "Utils/ProjectManager.h"
#include "EntityUUID.h"
#include <Assets/PrefabAsset.h>
#include <glm/gtx/matrix_decompose.hpp>

using namespace rv;

Scene::Scene(const std::string& sceneName) : m_Registry() , m_ScriptSystem(*this), m_CameraSystem(*this)
{
    m_SceneName = sceneName;
    m_RootEntity = m_Registry.create();
    Entity entity(m_RootEntity, this);
    entity.AddComponent<TransformComponent>(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    entity.AddComponent<SceneTreeComponent>();
    entity.AddComponent<TagComponent>(sceneName);
    entity.AddComponent<UUIDComponent>(EntityUUIDGenerator::Generate());
    m_EntityMap[entity.GetUUID()] = (entt::entity)m_RootEntity;
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
    m_ScriptSystem.OnStart(m_Registry);
}

void Scene::OnUpdate(float dt)
{
    m_ScriptSystem.OnUpdate(m_Registry, dt);
}

void Scene::OnStop()
{
    m_ScriptSystem.OnStop(m_Registry);
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



void Scene::Update() 
{                
    if (m_State == SceneState::PLAY)
        OnUpdate(Time::GetDeltaTime());
    UpdateHierarchy(); 
    m_CameraSystem.Update();
}

entt::registry& Scene::GetRegistry() { return m_Registry; }

void Scene::UpdateHierarchy() {
    auto view = m_Registry.view<SceneTreeComponent, TransformComponent>();

    UpdateNodeRecursive(m_RootEntity, glm::mat4(1.0f));
    //if(GetComponent<TransformComponent>(root).IsDirty()) // child may be dirty
}

void Scene::UpdateNodeRecursive(entt::entity e, const glm::mat4& parentWorld)
{
    auto& transform = GetComponent<TransformComponent>(e);

    glm::mat4 localMatrix = transform.GetLocalMatrix();
    glm::mat4 worldMatrix = parentWorld * localMatrix;

    glm::vec3 scale, skew, translation;
    glm::quat rotation;
    glm::vec4 perspective;
    glm::decompose(worldMatrix, scale, rotation, translation, skew, perspective);

    transform.SetWorldTransform(translation, rotation, scale);

    if (HasComponent<MeshRendererComponent>(e))
    {
        auto& meshRenderer = GetComponent<MeshRendererComponent>(e);
        meshRenderer.worldAABB = meshRenderer.localAABB.CalculateWorldAABB(worldMatrix);
    }

    //transform.ClearDirty();

    if (HasComponent<SceneTreeComponent>(e))
    {
        auto& sceneTree = GetComponent<SceneTreeComponent>(e);
        for (auto child : sceneTree.children)
        {
            if (m_Registry.valid(child))
            {
                UpdateNodeRecursive(child, worldMatrix);
            }
        }
    }
}

void Scene::SetParent(entt::entity child, entt::entity parent)
{
    if (child == entt::null || !m_Registry.valid(child)) return;

    if (parent != entt::null && !HasComponent<SceneTreeComponent>(parent))
        AddComponent<SceneTreeComponent>(parent);

    auto& childNode = GetComponent<SceneTreeComponent>(child);
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

    if (childTree.parent != entt::null)
    {
        auto& oldParentTree = GetComponent<SceneTreeComponent>(childTree.parent);
        std::erase(oldParentTree.children, child);
    }

    childTree.parent = parent;

    if (parent != entt::null)
    {
        GetComponent<SceneTreeComponent>(parent).children.push_back(child);
    }
}

void Scene::RemoveParent(entt::entity child)
{
    SetParent(child, m_RootEntity);
}




Entity Scene::Instantiate(const AssetUUID& prefabUUID)
{
    Ref<PrefabAsset> prefab = AssetRegistry::GetAsset<PrefabAsset>(prefabUUID);
    unsigned int entityCount = prefab->GetMetaAs<PrefabMeta>()->entityCount;

    const std::vector<std::byte>& buffer = prefab->GetData();
    const std::byte* ptr = buffer.data();
    const std::byte* end = buffer.data() + buffer.size();

    std::unordered_map<EntityUUID, Entity> uuidToEntity;
    Entity rootEntity;

    for (unsigned int i = 0; i < entityCount; i++)
    {
        Entity e = CreateEntityRaw();

        unsigned int componentCount;
        memcpy(&componentCount, ptr, sizeof(unsigned int));
        ptr += sizeof(unsigned int);

        for (unsigned int j = 0; j < componentCount; j++)
        {
            ComponentHeader header;
            memcpy(&header, ptr, (sizeof(ComponentHeader)));
            ptr += sizeof(ComponentHeader);

            ComponentType compType = static_cast<ComponentType>(header.type);
            switch (compType)
            {
            case ComponentType::DirectionalLight:
            {
                DirectionalLightComponent dirComp;
                DeserializeBin_DirectionalLightComp(ptr, dirComp);
                AddComponent<DirectionalLightComponent>(e, std::move(dirComp));
                break;
            }
            case ComponentType::Material:
            {
                MaterialComponent matComp;
                DeserializeBin_MaterialComp(ptr, matComp);
                AddComponent<MaterialComponent>(e, std::move(matComp));
                break;
            }
            case ComponentType::Mesh:
            {
                auto& mshComp = AddComponent<MeshComponent>(e);
                DeserializeBin_MeshComp(ptr, mshComp);
                break;
            }
            case ComponentType::MeshRenderer:
            {
                AddComponent<MeshRendererComponent>(e);
                auto& comp = GetComponent<MeshRendererComponent>(e);
                DeserializeBin_MeshRendererComp(ptr, comp);
                if (HasComponent<MeshComponent>(e))
                {
                    Ref<MeshAsset> mesh = GetComponent<MeshComponent>(e).GetMesh();
                    comp.RecreateFromMesh(mesh);
                }
                break;
            }
            case ComponentType::PointLight:
            {
                PointLightComponent plgComp;
                DeserializeBin_PointLightComp(ptr, plgComp);
                AddComponent<PointLightComponent>(e, std::move(plgComp));
                break;
            }
            case ComponentType::Camera:
            {
                auto& comp = AddComponent<CameraComponent>(e);
                DeserializeBin_CameraComp(ptr, comp);
                break;
            }
            case ComponentType::SceneTree:
            {
                auto& comp = GetComponent<SceneTreeComponent>(e);
                DeserializeBin_SceneTreeComp(ptr, comp);
                break;
            }
            case ComponentType::Tag:
            {
                auto& comp = AddComponent<TagComponent>(e);
                DeserializeBin_TagComp(ptr, comp);
                break;
            }
            case ComponentType::Transform:
            {
                auto& comp = GetComponent<TransformComponent>(e);
                DeserializeBin_TransformComp(ptr, comp);
                break;
            }
            case ComponentType::UUID:
            {
                auto& comp = GetComponent<UUIDComponent>(e);
                EntityUUID oldUUID;
                DeserializeBin_UUIDComp(ptr, comp);

                oldUUID = comp.uuid;

                comp.uuid = EntityUUIDGenerator::Generate();

                uuidToEntity[oldUUID] = e;

                m_EntityMap[comp.uuid] = (entt::entity)e;
                break;
            }
            
            default:
            {
                LOG_ERROR("Undefined component type id");
                break;
            }
            }
        }
    }


    for (auto& [oldUUID, entity] : uuidToEntity)
    {
        if (!HasComponent<SceneTreeComponent>(entity)) continue;

        auto& tree = GetComponent<SceneTreeComponent>(entity);

        if (tree.parentUUID != 0 && uuidToEntity.contains(tree.parentUUID))
        {
            entt::entity parent = uuidToEntity[tree.parentUUID].GetHandle();

            tree.parent = parent;

            auto& pnode = GetComponent<SceneTreeComponent>(parent);
            pnode.children.push_back(entity);

            tree.parentUUID = GetComponent<UUIDComponent>(parent).uuid;
        }
        else
        {
            tree.parent = m_RootEntity;
            tree.parentUUID = GetComponent<UUIDComponent>(m_RootEntity).uuid;

            auto& rootNode = GetComponent<SceneTreeComponent>(m_RootEntity);
            rootNode.children.push_back(entity);
            rootNode.childrenUUIDs.push_back(GetComponent<UUIDComponent>(entity).uuid);

            rootEntity = entity;
        }
    }

    if (rootEntity.GetHandle() == entt::null)
    {
        for (auto& [oldUUID, entity] : uuidToEntity)
        {
            if (!HasComponent<SceneTreeComponent>(entity)) continue;

            auto& tree = GetComponent<SceneTreeComponent>(entity);
            if (tree.parent == m_RootEntity)
            {
                rootEntity = entity;
                break;
            }
        }
    }

    if (rootEntity.GetHandle() != entt::null)
    {
        AddComponent<PrefabComponent>(rootEntity, prefabUUID);
    }

    UpdateHierarchy();

    return rootEntity;
}

[[nodiscard]] unsigned int Scene::GetComponentCount(entt::entity entity) noexcept
{
    auto& registry = GetRegistry();
    unsigned int count = 0;

    if (registry.any_of<TransformComponent>(entity)) count++;
    if (registry.any_of<MeshComponent>(entity)) count++;
    if (registry.any_of<DirectionalLightComponent>(entity)) count++;
    if (registry.any_of<MaterialComponent>(entity)) count++;
    if (registry.any_of<MeshRendererComponent>(entity)) count++;
    if (registry.any_of<PointLightComponent>(entity)) count++;
    if (registry.any_of<SceneTreeComponent>(entity)) count++;
    if (registry.any_of<SpotLightComponent>(entity)) count++;
    if (registry.any_of<TagComponent>(entity)) count++;
    if (registry.any_of<UUIDComponent>(entity)) count++;
    if (registry.any_of<PrefabComponent>(entity)) count++;
    if (registry.any_of<CameraComponent>(entity)) count++;

    return count;
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
