#pragma once

#include "entt/entt.h"
#include "Core/Ref.h"
#include <string>
#include "json.hpp"

class Scene;
class PrefabAsset;


class PrefabImporter 
{
public:

    static Ref<PrefabAsset> CreatePrefabAsset(const std::string& path, Scene& scene, entt::entity& rootEntity);
private:
    static void SerializeEntityRecursively(entt::entity& e, entt::entity& rootEntity, Scene& scene, std::vector<std::byte>& buffer,nlohmann::json& j);
    static unsigned int CountEntitiesRecursively(Scene& scene, entt::entity e);
};