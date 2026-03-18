#pragma once
#include "Scene.h"

namespace rv {

class SceneSerializer
{
public:
	void SaveScene(Scene& scene, const std::string& path);
	void LoadScene(Scene& scene, const std::string& path);

    static json SerializeEntity(Scene& scene, entt::entity e);
    static void DeserializeEntity(Scene& scene, const json& entityJson,
        std::unordered_map<EntityUUID, entt::entity>& uuidToEntity);

private:
    void CollectChildrenRecursively(Scene& scene,entt::entity e,std::unordered_set<entt::entity>& out);
};

}