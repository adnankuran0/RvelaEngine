#include "rvelapch.h"
#include "PrefabImporter.h"
#include "Asset/Types/PrefabAsset.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/SceneSerializer.h"
#include "Scene/Components/PrefabComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/TagComponent.h"
#include "Scene/Components/SceneTreeComponent.h"
#include "Core/Log.h"
#include "Asset/AssetManager.h"
#include "EditorSelection.h"
#include <nlohmann/json.hpp>
#include <fstream>

using namespace rv;
using json = nlohmann::json;

Ref<PrefabAsset> PrefabImporter::CreatePrefabAsset(
    const std::filesystem::path& path,
    const AssetUUID& uuid,
    Scene& scene,
    entt::entity rootEntity)
{
    json prefabJson;
    prefabJson["Entities"] = json::array();

    SerializeEntityRecursively(rootEntity, rootEntity, scene, prefabJson["Entities"]);

    // reset root transform
    for (auto& entityJson : prefabJson["Entities"])
    {
        if (entityJson.contains("_isRoot") && entityJson["_isRoot"] == true)
        {
            if (entityJson.contains("Transform"))
            {
                entityJson["Transform"]["position"] = { 0.0f, 0.0f, 0.0f };
                entityJson["Transform"]["rotation"] = { 0.0f, 0.0f, 0.0f };
            }
            entityJson.erase("ParentUUID");
            break;
        }
    }

    std::string jsonStr = prefabJson.dump(4);

    std::ofstream file(path);
    if (!file)
    {
        LOG_ERROR("Cannot write: {}", path.string());
        return nullptr;
    }
    file << jsonStr;

    auto asset = CreateRef<PrefabAsset>(uuid);
    asset->m_JSON = std::move(jsonStr);

    return asset;
}

Ref<PrefabAsset> PrefabImporter::CreateOrUpdatePrefabAsset(
    const std::filesystem::path& path,
    const AssetUUID& uuid,
    Scene& scene,
    entt::entity rootEntity)
{
    if (!std::filesystem::exists(path))
    {
        return CreatePrefabAsset(path, uuid, scene, rootEntity);
    }

    json newPrefabJson;
    newPrefabJson["Entities"] = json::array();
    SerializeEntityRecursively(rootEntity, rootEntity, scene, newPrefabJson["Entities"]);

    for (auto& entityJson : newPrefabJson["Entities"])
    {
        if (entityJson.contains("_isRoot") && entityJson["_isRoot"] == true)
        {
            if (entityJson.contains("Transform"))
            {
                entityJson["Transform"]["position"] = { 0.0f, 0.0f, 0.0f };
                entityJson["Transform"]["rotation"] = { 0.0f, 0.0f, 0.0f };
            }
            entityJson.erase("ParentUUID");
            break;
        }
    }

    json existingJson;
    bool readOk = false;
    try {
        std::ifstream inFile(path);
        if (inFile.is_open())
        {
            inFile >> existingJson;
            readOk = existingJson.contains("Entities") && existingJson["Entities"].is_array();
        }
    } catch (const std::exception& e) {
        LOG_WARN("[PrefabImporter] Could not parse existing prefab: {}, re-creating.", e.what());
        readOk = false;
    }

    if (!readOk)
    {
        return CreatePrefabAsset(path, uuid, scene, rootEntity);
    }

    auto& existingEntities = existingJson["Entities"];
    auto& newEntities = newPrefabJson["Entities"];

    std::vector<bool> newMatched(newEntities.size(), false);

    for (auto& existingEnt : existingEntities)
    {
        bool isExistingRoot = existingEnt.value("_isRoot", false);
        std::string existingTag = "";
        if (existingEnt.contains("Tag") && existingEnt["Tag"].contains("tag"))
            existingTag = existingEnt["Tag"]["tag"].get<std::string>();

        int matchIdx = -1;
        if (isExistingRoot)
        {
            for (size_t ni = 0; ni < newEntities.size(); ++ni)
            {
                if (!newMatched[ni] && newEntities[ni].value("_isRoot", false))
                {
                    matchIdx = static_cast<int>(ni);
                    break;
                }
            }
        }
        else if (!existingTag.empty())
        {
            for (size_t ni = 0; ni < newEntities.size(); ++ni)
            {
                if (!newMatched[ni] && newEntities[ni].contains("Tag") &&
                    newEntities[ni]["Tag"].contains("tag") &&
                    newEntities[ni]["Tag"]["tag"] == existingTag)
                {
                    matchIdx = static_cast<int>(ni);
                    break;
                }
            }
        }

        if (matchIdx >= 0)
        {
            newMatched[matchIdx] = true;
            const auto& newEnt = newEntities[matchIdx];

            if (newEnt.contains("Mesh"))
                existingEnt["Mesh"] = newEnt["Mesh"];
            else
                existingEnt.erase("Mesh");

            if (newEnt.contains("SkeletalMeshComponent"))
                existingEnt["SkeletalMeshComponent"] = newEnt["SkeletalMeshComponent"];
            else
                existingEnt.erase("SkeletalMeshComponent");

            if (newEnt.contains("SkeletonComponent"))
                existingEnt["SkeletonComponent"] = newEnt["SkeletonComponent"];
            else
                existingEnt.erase("SkeletonComponent");

            if (newEnt.contains("AnimatorComponent"))
                existingEnt["AnimatorComponent"] = newEnt["AnimatorComponent"];

            if (newEnt.contains("Material") && !existingEnt.contains("Material"))
                existingEnt["Material"] = newEnt["Material"];

        }
    }

    for (size_t ni = 0; ni < newEntities.size(); ++ni)
    {
        if (!newMatched[ni])
        {
            existingEntities.push_back(newEntities[ni]);
        }
    }

    std::string jsonStr = existingJson.dump(4);
    std::ofstream outFile(path);
    if (!outFile)
    {
        LOG_ERROR("[PrefabImporter] Cannot write updated prefab: {}", path.string());
        return nullptr;
    }
    outFile << jsonStr;

    auto asset = CreateRef<PrefabAsset>(uuid);
    asset->m_JSON = std::move(jsonStr);
    return asset;
}

void PrefabImporter::SerializeEntityRecursively(
    entt::entity e,
    entt::entity rootEntity,
    Scene& scene,
    json& outEntities)
{
    json entityJson = SceneSerializer::SerializeEntity(scene, e, /*serializePrefabAsInstance=*/false);
    
    if (e == rootEntity)
        entityJson["_isRoot"] = true;

    outEntities.push_back(entityJson);

    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& children = scene.GetComponent<SceneTreeComponent>(e).children;
        for (auto child : children)
            SerializeEntityRecursively(child, rootEntity, scene, outEntities);
    }
}

unsigned int PrefabImporter::CountEntitiesRecursively(Scene& scene, entt::entity e)
{
    unsigned int count = 1;
    if (scene.HasComponent<SceneTreeComponent>(e))
        for (auto child : scene.GetComponent<SceneTreeComponent>(e).children)
            count += CountEntitiesRecursively(scene, child);
    return count;
}

bool PrefabImporter::ApplyPrefab(Scene& scene, entt::entity rootEntity)
{
    if (rootEntity == entt::null || !scene.GetRegistry().valid(rootEntity))
        return false;

    if (!scene.HasComponent<PrefabComponent>(rootEntity))
        return false;

    AssetUUID prefabUUID = scene.GetComponent<PrefabComponent>(rootEntity).GetPrefabID();
    if (!prefabUUID.IsValid())
        return false;

    AssetRegistry& reg = AssetManager::Get().GetRegistry();
    std::filesystem::path prefabPath = reg.GetPath(prefabUUID);
    if (prefabPath.empty())
    {
        LOG_ERROR("[PrefabImporter::ApplyPrefab] Cannot find path for prefab UUID: {}", prefabUUID.ToString());
        return false;
    }

    Ref<PrefabAsset> prefab = CreatePrefabAsset(prefabPath, prefabUUID, scene, rootEntity);
    if (!prefab)
    {
        LOG_ERROR("[PrefabImporter::ApplyPrefab] Failed to save prefab to: {}", prefabPath.string());
        return false;
    }

    // Invalidate cached asset so any future loads get the fresh file from disk
    AssetManager::Get().Unload(prefabUUID);

    // Propagate changes to all other instances of this prefab in the active scene, preserving their overrides
    std::vector<entt::entity> otherInstances;
    auto view = scene.GetRegistry().view<PrefabComponent>();
    for (auto e : view)
    {
        if (e != rootEntity && view.get<PrefabComponent>(e).GetPrefabID() == prefabUUID)
        {
            otherInstances.push_back(e);
        }
    }

    for (auto other : otherInstances)
    {
        ReloadPrefab(scene, other);
    }

    EditorSelection::Get().Validate(scene.GetRegistry());

    return true;
}

bool PrefabImporter::RevertPrefab(Scene& scene, entt::entity rootEntity, bool preserveTag)
{
    if (rootEntity == entt::null || !scene.GetRegistry().valid(rootEntity))
        return false;

    if (!scene.HasComponent<PrefabComponent>(rootEntity))
        return false;

    AssetUUID prefabUUID = scene.GetComponent<PrefabComponent>(rootEntity).GetPrefabID();
    if (!prefabUUID.IsValid())
        return false;

    AssetRegistry& reg = AssetManager::Get().GetRegistry();
    std::filesystem::path prefabPath = reg.GetPath(prefabUUID);

    // Unload existing cached prefab asset so it reloads fresh from disk
    AssetManager::Get().Unload(prefabUUID);

    // Preserve root transform and parent
    glm::vec3 pos(0.0f);
    glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale(1.0f);
    if (scene.HasComponent<TransformComponent>(rootEntity))
    {
        auto& tc = scene.GetComponent<TransformComponent>(rootEntity);
        pos = tc.GetPosition();
        rot = tc.GetRotation();
        scale = tc.GetScale();
    }

    entt::entity parentEntity = entt::null;
    if (scene.HasComponent<SceneTreeComponent>(rootEntity))
    {
        parentEntity = scene.GetComponent<SceneTreeComponent>(rootEntity).parent;
    }

    std::string originalTag;
    if (preserveTag && scene.HasComponent<TagComponent>(rootEntity))
    {
        originalTag = scene.GetComponent<TagComponent>(rootEntity).tag;
    }

    bool wasSelected = EditorSelection::Get().IsSelected(rootEntity);

    scene.DestroyEntity(rootEntity);

    // Instantiate fresh from disk
    Entity newInstance = scene.Instantiate(prefabUUID, pos, rot, parentEntity);
    if (newInstance.GetHandle() != entt::null)
    {
        if (scene.HasComponent<TransformComponent>(newInstance.GetHandle()))
        {
            auto& tc = scene.GetComponent<TransformComponent>(newInstance.GetHandle());
            tc.SetScale(scale);
            tc.SetDirty();
        }

        if (preserveTag && !originalTag.empty() && scene.HasComponent<TagComponent>(newInstance.GetHandle()))
        {
            scene.GetComponent<TagComponent>(newInstance.GetHandle()).tag = originalTag;
        }

        if (wasSelected)
        {
            EditorSelection::Get().Remove(rootEntity);
            EditorSelection::Get().Select(newInstance.GetHandle());
        }

        return true;
    }

    LOG_ERROR("[PrefabImporter::RevertPrefab] Failed to re-instantiate prefab UUID: {}", prefabUUID.ToString());
    return false;
}

bool PrefabImporter::ReloadPrefab(Scene& scene, entt::entity rootEntity, const json* precomputedOverrides)
{
    if (rootEntity == entt::null || !scene.GetRegistry().valid(rootEntity))
        return false;

    if (!scene.HasComponent<PrefabComponent>(rootEntity))
        return false;

    AssetUUID prefabUUID = scene.GetComponent<PrefabComponent>(rootEntity).GetPrefabID();
    if (!prefabUUID.IsValid())
        return false;

    json overrides = json::array();
    if (precomputedOverrides)
    {
        overrides = *precomputedOverrides;
    }
    else
    {
        overrides = SceneSerializer::ComputePrefabOverrides(scene, rootEntity);
        if (overrides.empty() && scene.GetComponent<PrefabComponent>(rootEntity).HasOverrides())
        {
            overrides = scene.GetComponent<PrefabComponent>(rootEntity).GetOverrides();
        }
    }

    AssetManager::Get().Unload(prefabUUID);

    glm::vec3 pos(0.0f);
    glm::quat rot(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale(1.0f);
    if (scene.HasComponent<TransformComponent>(rootEntity))
    {
        auto& tc = scene.GetComponent<TransformComponent>(rootEntity);
        pos = tc.GetPosition();
        rot = tc.GetRotation();
        scale = tc.GetScale();
    }

    entt::entity parentEntity = entt::null;
    if (scene.HasComponent<SceneTreeComponent>(rootEntity))
    {
        parentEntity = scene.GetComponent<SceneTreeComponent>(rootEntity).parent;
    }

    std::string originalTag;
    if (scene.HasComponent<TagComponent>(rootEntity))
    {
        originalTag = scene.GetComponent<TagComponent>(rootEntity).tag;
    }

    bool wasSelected = EditorSelection::Get().IsSelected(rootEntity);

    scene.DestroyEntity(rootEntity);

    Entity newInstance = scene.Instantiate(prefabUUID, pos, rot, parentEntity, overrides);
    if (newInstance.GetHandle() != entt::null)
    {
        if (scene.HasComponent<TransformComponent>(newInstance.GetHandle()))
        {
            auto& tc = scene.GetComponent<TransformComponent>(newInstance.GetHandle());
            tc.SetScale(scale);
            tc.SetDirty();
        }

        if (!originalTag.empty() && scene.HasComponent<TagComponent>(newInstance.GetHandle()))
        {
            scene.GetComponent<TagComponent>(newInstance.GetHandle()).tag = originalTag;
        }

        if (wasSelected)
        {
            EditorSelection::Get().Remove(rootEntity);
            EditorSelection::Get().Select(newInstance.GetHandle());
        }

        return true;
    }

    LOG_ERROR("[PrefabImporter::ReloadPrefab] Failed to re-instantiate prefab UUID: {}", prefabUUID.ToString());
    return false;
}
