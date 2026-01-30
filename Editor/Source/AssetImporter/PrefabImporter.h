#pragma once
#include "AssetImporter.h"
#include "Core/Ref.h"
#include "Assets/PrefabAsset.h"
#include "Scene/Scene.h"
#include "Utils/Serializer.h"
#include "Core/Singleton.h"


class PrefabImporter 
{
public:

    static Ref<PrefabAsset> CreatePrefabAsset(const std::string& path, Scene& scene, entt::entity& rootEntity);
private:
    static void SerializeEntityRecursively(entt::entity& e, entt::entity& rootEntity, Scene& scene, std::vector<std::byte>& buffer,json& j);
    static unsigned int CountEntitiesRecursively(Scene& scene, entt::entity e);
};