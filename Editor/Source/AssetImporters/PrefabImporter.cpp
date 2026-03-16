#include "rvelapch.h"
#include "PrefabImporter.h"
#include "Asset/Types/PrefabAsset.h"
#include "Scene/Scene.h"
#include "Scene/SceneSerializer.h"
#include "Core/Log.h"
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
                entityJson["Transform"]["rotation"] = { 1.0f, 0.0f, 0.0f, 0.0f };
            }
            entityJson.erase("_isRoot");
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

void PrefabImporter::SerializeEntityRecursively(
    entt::entity e,
    entt::entity rootEntity,
    Scene& scene,
    json& outEntities)
{
    json entityJson = SceneSerializer::SerializeEntity(scene, e);

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