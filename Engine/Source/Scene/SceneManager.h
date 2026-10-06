#pragma once
#include "Scene.h"
#include "SceneSerializer.h"
#include "Asset/AssetUUID.h"

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

    void PlayScene();
    void PauseScene();
    void StopScene();

    void SaveScene(const std::string& path);
    void SaveScene(Scene& scene, const std::string& path);
    bool LoadScene(const std::string& path);
    bool LoadScene(const AssetUUID& sceneAsset);

    void Update();
    void FixedUpdate();
    void LateUpdate();
private:
    std::string m_PendingScenePath = "";
    bool m_HasPendingScene = false; 
    std::unique_ptr<Scene> m_CurrentScene = nullptr;
    SceneSerializer m_SceneSerializer;

    bool m_HasSnapshot = false;
    json m_SceneSnapshot;
    std::string m_SnapshotScenePath = "";
    std::string m_SnapshotSceneName = "";
};

}
