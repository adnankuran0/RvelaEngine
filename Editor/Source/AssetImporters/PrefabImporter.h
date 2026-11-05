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

    static Ref<PrefabAsset> CreateOrUpdatePrefabAsset(
        const std::filesystem::path& path,
        const AssetUUID& uuid,
        Scene& scene,
        entt::entity rootEntity);

    static bool ApplyPrefab(Scene& scene, entt::entity rootEntity);
    static bool RevertPrefab(Scene& scene, entt::entity rootEntity, bool preserveTag = false);
    static bool ReloadPrefab(Scene& scene, entt::entity rootEntity, const nlohmann::json* precomputedOverrides = nullptr);

    static void SerializeEntityRecursively(
        entt::entity e,
        entt::entity rootEntity,
        Scene& scene,
        nlohmann::json& outEntities);

private:
    static unsigned int CountEntitiesRecursively(Scene& scene, entt::entity e);
};

}
