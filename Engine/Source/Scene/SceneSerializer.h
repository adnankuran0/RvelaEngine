#pragma once
#include "Scene.h"

namespace rv {

class SceneSerializer
{
public:
	void SaveScene(Scene& scene, const std::string& path);
	void LoadScene(Scene& scene, const std::string& path);

    static json SerializeEntity(Scene& scene, entt::entity e, bool serializePrefabAsInstance = true);
    static void DeserializeEntity(Scene& scene, const json& entityJson,
        std::unordered_map<EntityUUID, entt::entity>& uuidToEntity);
    static void DeserializeEntityComponents(Scene& scene, entt::entity handle, const json& entityJson);
    static Entity CloneEntity(Scene& scene, entt::entity sourceHandle);
    static std::string GenerateUniqueName(Scene& scene, const std::string& baseName);

private:
    void SerializeHierarchyRecursively(Scene& scene, entt::entity current, json& outEntitiesArray, const std::unordered_set<entt::entity>& prefabChildren);
    void CollectChildrenRecursively(Scene& scene,entt::entity e,std::unordered_set<entt::entity>& out);
};

}