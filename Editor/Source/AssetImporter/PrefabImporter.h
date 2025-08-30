#pragma once
#include "AssetImporter.h"
#include "Core/Ref.h"
#include "Assets/PrefabAsset.h"
#include "Scene/Scene.h"
#include "Utils/Serializer.h"
#include "Core/Singleton.h"


class PrefabImporter : public Singleton<PrefabImporter>
{
public:

    Ref<PrefabAsset> CreatePrefabAsset(const std::string& path, Scene& scene, entt::entity& rootEntity);
private:
    void SerializeEntityRecursively(entt::entity& e, Scene& scene, std::vector<std::byte>& buffer,json& j);
    unsigned int GetComponentCount(Scene& scene, entt::entity& entity);
    unsigned int CountEntitiesRecursively(Scene& scene, entt::entity e);
};