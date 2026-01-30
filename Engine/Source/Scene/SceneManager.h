#pragma once
#include "Scene.h"
#include "Entity.h"

class SceneManager
{
public:
    void SaveScene(Scene& scene, const std::string& path);
    void LoadScene(Scene& scene, const std::string& path);

private:
    Scene* m_CurrentScene;

    unsigned int CountEntitiesRecursively(entt::entity& rootEntity);
    void SerializeBinEntityRecursively(entt::entity& e, Scene& scene, std::vector<std::byte>& buffer);
    json SerializeEntity(Scene& scene, entt::entity e);
};