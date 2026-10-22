#pragma once
#include "Scene.h"
#include "SceneSerializer.h"
#include "Asset/AssetUUID.h"
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <memory>
#include <string>

namespace rv {

class SceneManager
{
public:
    enum class AsyncLoadStatus
    {
        None = 0,
        Loading,
        Loaded,
        Failed,
        Cancelled
    };

    struct AsyncSceneLoadTask
    {
        uint32_t id = 0;
        std::string path;
        std::string sceneName;
        std::atomic<AsyncLoadStatus> status{ AsyncLoadStatus::None };
        std::atomic<float> progress{ 0.0f };
        std::string errorMessage;
        bool autoActivate = false;
        std::atomic<bool> cancelled{ false };

        std::thread workerThread;
        std::atomic<bool> jsonParsed{ false };
        json parsedJson;

        std::unique_ptr<Scene> scene = nullptr;
    };

    SceneManager() = default;
    ~SceneManager();

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

    uint32_t LoadSceneAsync(const std::string& path, bool autoActivate = false);
    uint32_t LoadSceneAsync(const AssetUUID& sceneAsset, bool autoActivate = false);

    uint32_t ChangeSceneAsync(const std::string& path);
    uint32_t ChangeSceneAsync(const AssetUUID& sceneAsset);

    bool IsSceneLoaded(uint32_t taskId) const;
    bool IsSceneLoaded(const std::string& path) const;
    bool IsSceneLoaded(const AssetUUID& sceneAsset) const;

    float GetLoadProgress(uint32_t taskId) const;
    float GetLoadProgress(const std::string& path) const;
    float GetLoadProgress(const AssetUUID& sceneAsset) const;

    AsyncLoadStatus GetLoadStatus(uint32_t taskId) const;
    AsyncLoadStatus GetLoadStatus(const std::string& path) const;
    AsyncLoadStatus GetLoadStatus(const AssetUUID& sceneAsset) const;

    bool ChangeToLoadedScene(uint32_t taskId);
    bool ChangeToLoadedScene(const std::string& path);
    bool ChangeToLoadedScene(const AssetUUID& sceneAsset);

    bool UnloadLoadedScene(uint32_t taskId);
    bool UnloadLoadedScene(const std::string& path);
    bool UnloadLoadedScene(const AssetUUID& sceneAsset);

    bool CancelAsyncLoad(uint32_t taskId);
    bool CancelAsyncLoad(const std::string& path);
    bool CancelAsyncLoad(const AssetUUID& sceneAsset);

    std::vector<std::string> GetLoadedScenePaths() const;
    void ClearAsyncLoads();

    bool ResolveScenePath(const std::string& input, std::filesystem::path& resolved) const;
    bool ResolveScenePath(const AssetUUID& sceneAsset, std::filesystem::path& resolved) const;

    void Update();
    void FixedUpdate();
    void LateUpdate();

    void CaptureAutoloadStates(Scene& scene);
    void RestoreAutoloadStates(Scene& scene);

private:
    void ProcessAsyncLoads();
    void ActivateSceneInternal(std::unique_ptr<Scene> scenePtr);
    void ActivateLoadedSceneInternal(uint32_t taskId);
    std::shared_ptr<AsyncSceneLoadTask> FindAsyncTask(uint32_t taskId) const;
    std::shared_ptr<AsyncSceneLoadTask> FindAsyncTask(const std::string& path) const;
    std::shared_ptr<AsyncSceneLoadTask> FindAsyncTask(const AssetUUID& sceneAsset) const;

private:
    std::string m_PendingScenePath = "";
    bool m_HasPendingScene = false; 

    bool m_HasPendingLoadedScene = false;
    uint32_t m_PendingLoadedTaskId = 0;

    std::unique_ptr<Scene> m_CurrentScene = nullptr;
    SceneSerializer m_SceneSerializer;

    bool m_HasSnapshot = false;
    json m_SceneSnapshot;
    std::string m_SnapshotScenePath = "";
    std::string m_SnapshotSceneName = "";
    bool m_SnapshotIsDirty = false;

    std::unordered_map<std::string, json> m_AutoloadStates;

    uint32_t m_NextAsyncTaskId = 0;
    mutable std::mutex m_AsyncMutex;
    std::vector<std::shared_ptr<AsyncSceneLoadTask>> m_AsyncTasks;
};

}
