#pragma once
#include "Scene.h"
#include "SceneSerializer.h"

namespace rv {

class SceneManager
{
public:

    void Init();

    std::unique_ptr<Scene> CreateScene(const std::string& sceneName)
    {
        return std::make_unique<Scene>(sceneName);
    }

    Scene& GetActiveScene()
    {
        assert(m_CurrentScene);
        return *m_CurrentScene;
    }
    void SetActiveScene(std::unique_ptr<Scene>&& newScene)
    {
        m_CurrentScene = std::move(newScene);
    }

    void SaveScene(const std::string& path);
    void LoadScene(const std::string& path);

    void Update();
private:
    std::string m_PendingScenePath = "";
    bool m_HasPendingScene = false; 
    std::unique_ptr<Scene> m_CurrentScene = nullptr;
    SceneSerializer m_SceneSerializer;
    
};

}