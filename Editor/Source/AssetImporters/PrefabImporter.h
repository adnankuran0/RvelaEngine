#pragma once
#include <filesystem>
#include "Core/Ref.h"
#include "Asset/AssetUUID.h"
#include <entt/entt.h>
#include "nlohmann/json.hpp"

namespace rv {

class Scene;
class PrefabAsset;
class AssetRegistry;

class PrefabImporter
{
public:
    static Ref<PrefabAsset> CreatePrefabAsset(
        const std::filesystem::path& path,
        const AssetUUID& uuid,
        Scene& scene,
        entt::entity rootEntity);

private:
    static void SerializeEntityRecursively(
        entt::entity e,
        entt::entity rootEntity,
        Scene& scene,
        nlohmann::json& outEntities);

    static unsigned int CountEntitiesRecursively(Scene& scene, entt::entity e);
};

}