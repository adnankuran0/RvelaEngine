#pragma once
#include "Scene.h"
#include "Entity.h"

class SceneManager
{
public:

    std::unique_ptr<Scene> CreateScene(const std::string& sceneName)
    {
        return std::make_unique<Scene>(sceneName);
    }
    Scene& GetActiveScene()
    {
        assert(m_CurrentScene);
        return *m_CurrentScene;
    }
    void SetActiveScene(std::unique_ptr<Scene> newScene)
    {
        m_CurrentScene = std::move(newScene);
    }


    void SaveScene(const std::string& path);
    void LoadScene(const std::string& path);
private:
    std::unique_ptr<Scene> m_CurrentScene = nullptr;

    unsigned int CountEntitiesRecursively(entt::entity& rootEntity);
    json SerializeEntity(Scene& scene, entt::entity e);
};