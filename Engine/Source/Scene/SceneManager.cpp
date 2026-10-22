#include "rvelapch.h"
#include "SceneManager.h"
#include "Entity.h"
#include "json.hpp"
#include "EntityUUID.h"  
#include "Core/Log.h"
#include <Utils/Serializer.h>
#include "Renderer/TextureCache.h"
#include "Asset/AssetManager.h"
#include "Components/AutoloadComponent.h"
#include "Components/ScriptComponent.h"
#include "Components/TransformComponent.h"
#include "Script/ScriptUtils.h"
#include <filesystem>

using namespace rv;

using json = nlohmann::json;

namespace
{
	bool ResolveSceneFile(const std::string& input, std::filesystem::path& resolved)
	{
		if (input.empty())
		{
			LOG_ERROR("Cannot change scene: path is empty");
			return false;
		}

		const std::filesystem::path requested(input);
		if (requested.extension() != ".rscene")
		{
			LOG_ERROR("Cannot change scene: expected a .rscene file, got '{}'", input);
			return false;
		}

		std::vector<std::filesystem::path> candidates;
		if (requested.is_absolute())
		{
			candidates.push_back(requested);
		}
		else
		{
			const auto& assetDir = AssetManager::Get().GetRegistry().GetAssetDir();
			if (!assetDir.empty())
			{
				candidates.push_back(assetDir / requested);
				candidates.push_back(assetDir.parent_path() / requested);
			}
			candidates.push_back(requested);
		}

		for (const auto& candidate : candidates)
		{
			std::error_code ec;
			if (!std::filesystem::is_regular_file(candidate, ec) || ec)
				continue;

			resolved = std::filesystem::absolute(candidate, ec);
			if (ec)
				resolved = candidate;
			resolved = resolved.lexically_normal();
			return true;
		}

		LOG_ERROR("Cannot change scene: file does not exist: {}", input);
		return false;
	}
}

SceneManager::~SceneManager()
{
	ClearAsyncLoads();
}

bool SceneManager::ResolveScenePath(const std::string& input, std::filesystem::path& resolved) const
{
	if (input.empty())
	{
		LOG_ERROR("Cannot resolve scene: path is empty");
		return false;
	}

	if (input.length() == 36 && input[8] == '-' && input[13] == '-' && input[18] == '-' && input[23] == '-')
	{
		AssetUUID uuid = AssetUUID::FromString(input);
		if (uuid.IsValid())
			return ResolveScenePath(uuid, resolved);
	}

	auto& registry = AssetManager::Get().GetRegistry();
	AssetUUID uuid = registry.GetUUID(input);
	if (uuid.IsValid())
	{
		return ResolveScenePath(uuid, resolved);
	}

	return ResolveSceneFile(input, resolved);
}

bool SceneManager::ResolveScenePath(const AssetUUID& sceneAsset, std::filesystem::path& resolved) const
{
	if (!sceneAsset.IsValid())
	{
		LOG_ERROR("Cannot change scene: AssetHandle is invalid");
		return false;
	}

	auto& registry = AssetManager::Get().GetRegistry();
	if (!registry.Exists(sceneAsset))
	{
		LOG_ERROR("Cannot change scene: AssetHandle is not registered: {}", sceneAsset.ToString());
		return false;
	}

	const std::filesystem::path path = registry.GetPath(sceneAsset);
	if (path.extension() != ".rscene")
	{
		LOG_ERROR("Cannot change scene: AssetHandle does not refer to a .rscene file: {}", sceneAsset.ToString());
		return false;
	}

	return ResolveSceneFile(path.string(), resolved);
}

void SceneManager::Init()
{
    SetActiveScene(CreateScene("EmptyScene"));
}

void SceneManager::PlayScene()
{
    if (!m_CurrentScene) return;

    if (m_CurrentScene->GetState() == SceneState::EDIT)
    {
        m_SceneSnapshot = m_SceneSerializer.SerializeScene(*m_CurrentScene);
        m_SnapshotScenePath = m_CurrentScene->GetPath();
        m_SnapshotSceneName = m_CurrentScene->GetName();
        m_SnapshotIsDirty = m_CurrentScene->IsDirty();
        m_HasSnapshot = true;

        m_CurrentScene->SetState(SceneState::PLAY);
    }
    else if (m_CurrentScene->GetState() == SceneState::PAUSE)
    {
        m_CurrentScene->SetState(SceneState::PLAY);
    }
}

void SceneManager::PauseScene()
{
    if (!m_CurrentScene) return;

    if (m_CurrentScene->GetState() == SceneState::PLAY)
    {
        m_CurrentScene->SetState(SceneState::PAUSE);
    }
}

void SceneManager::StopScene()
{
	m_HasPendingScene = false;
	m_PendingScenePath.clear();
	m_HasPendingLoadedScene = false;
	m_PendingLoadedTaskId = 0;
	m_AutoloadStates.clear();
	ClearAsyncLoads();

    if (!m_CurrentScene) return;

    if (m_CurrentScene->GetState() == SceneState::PLAY || m_CurrentScene->GetState() == SceneState::PAUSE)
    {
        m_CurrentScene->SetState(SceneState::EDIT);

        if (m_HasSnapshot)
        {
            std::unique_ptr<Scene> restoredScene = CreateScene(m_SnapshotSceneName.empty() ? "Untitled" : m_SnapshotSceneName);
            restoredScene->SetPath(m_SnapshotScenePath);
            m_SceneSerializer.DeserializeScene(*restoredScene, m_SceneSnapshot);
            restoredScene->SetDirty(m_SnapshotIsDirty);
            SetActiveScene(std::move(restoredScene));

            m_HasSnapshot = false;
            m_SceneSnapshot.clear();
        }
    }
}

void SceneManager::SaveScene(const std::string& path)
{
    m_SceneSerializer.SaveScene(GetActiveScene(), path);
}

void SceneManager::SaveScene(Scene& scene, const std::string& path)
{
    m_SceneSerializer.SaveScene(scene, path);
}

bool SceneManager::LoadScene(const std::string& path)
{
    std::filesystem::path resolved;
    if (!ResolveScenePath(path, resolved))
        return false;

    const std::string resolvedStr = resolved.string();
    if (ChangeToLoadedScene(resolvedStr))
        return true;

    m_PendingScenePath = resolvedStr;
    m_HasPendingScene = true;
    return true;
}

bool SceneManager::LoadScene(const AssetUUID& sceneAsset)
{
    std::filesystem::path resolved;
    if (!ResolveScenePath(sceneAsset, resolved))
        return false;

    return LoadScene(resolved.string());
}

uint32_t SceneManager::LoadSceneAsync(const std::string& path, bool autoActivate)
{
    std::filesystem::path resolved;
    if (!ResolveScenePath(path, resolved))
        return 0;

    const std::string resolvedStr = resolved.string();

    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        for (auto& task : m_AsyncTasks)
        {
            if (task->path == resolvedStr && !task->cancelled.load())
            {
                if (autoActivate)
                    task->autoActivate = true;

                if (task->status.load() == AsyncLoadStatus::Loaded && autoActivate)
                {
                    m_PendingLoadedTaskId = task->id;
                    m_HasPendingLoadedScene = true;
                }
                return task->id;
            }
        }
    }

    auto task = std::make_shared<AsyncSceneLoadTask>();
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        task->id = ++m_NextAsyncTaskId;
        task->path = resolvedStr;
        task->sceneName = resolved.stem().string();
        task->autoActivate = autoActivate;
        task->status.store(AsyncLoadStatus::Loading);
        task->progress.store(0.05f);
        m_AsyncTasks.push_back(task);
    }

    task->workerThread = std::thread([task]() {
        task->progress.store(0.1f);

        std::ifstream stream(task->path);
        if (!stream.is_open())
        {
            task->errorMessage = "Scene file could not be opened: " + task->path;
            task->progress.store(0.0f);
            task->status.store(AsyncLoadStatus::Failed);
            LOG_ERROR("{}", task->errorMessage);
            return;
        }

        if (task->cancelled.load())
        {
            task->status.store(AsyncLoadStatus::Cancelled);
            return;
        }

        task->progress.store(0.3f);
        try
        {
            stream >> task->parsedJson;
        }
        catch (const std::exception& e)
        {
            task->errorMessage = std::string("JSON parse error in ") + task->path + ": " + e.what();
            task->progress.store(0.0f);
            task->status.store(AsyncLoadStatus::Failed);
            LOG_ERROR("{}", task->errorMessage);
            return;
        }

        if (task->cancelled.load())
        {
            task->status.store(AsyncLoadStatus::Cancelled);
            return;
        }

        if (!task->parsedJson.is_object() || !task->parsedJson.contains("Entities") || !task->parsedJson["Entities"].is_array())
        {
            task->errorMessage = "Invalid scene data (expected an Entities array): " + task->path;
            task->progress.store(0.0f);
            task->status.store(AsyncLoadStatus::Failed);
            LOG_ERROR("{}", task->errorMessage);
            return;
        }

        task->progress.store(0.85f);
        task->jsonParsed.store(true);
    });

    return task->id;
}

uint32_t SceneManager::LoadSceneAsync(const AssetUUID& sceneAsset, bool autoActivate)
{
    std::filesystem::path resolved;
    if (!ResolveScenePath(sceneAsset, resolved))
        return 0;

    return LoadSceneAsync(resolved.string(), autoActivate);
}

uint32_t SceneManager::ChangeSceneAsync(const std::string& path)
{
    return LoadSceneAsync(path, true);
}

uint32_t SceneManager::ChangeSceneAsync(const AssetUUID& sceneAsset)
{
    return LoadSceneAsync(sceneAsset, true);
}

std::shared_ptr<SceneManager::AsyncSceneLoadTask> SceneManager::FindAsyncTask(uint32_t taskId) const
{
    if (taskId == 0) return nullptr;
    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    for (const auto& task : m_AsyncTasks)
    {
        if (task->id == taskId)
            return task;
    }
    return nullptr;
}

std::shared_ptr<SceneManager::AsyncSceneLoadTask> SceneManager::FindAsyncTask(const std::string& path) const
{
    std::filesystem::path resolved;
    if (!ResolveScenePath(path, resolved))
        return nullptr;

    const std::string resolvedStr = resolved.string();
    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    for (const auto& task : m_AsyncTasks)
    {
        if (task->path == resolvedStr)
            return task;
    }
    return nullptr;
}

std::shared_ptr<SceneManager::AsyncSceneLoadTask> SceneManager::FindAsyncTask(const AssetUUID& sceneAsset) const
{
    std::filesystem::path resolved;
    if (!ResolveScenePath(sceneAsset, resolved))
        return nullptr;

    return FindAsyncTask(resolved.string());
}

bool SceneManager::IsSceneLoaded(uint32_t taskId) const
{
    auto task = FindAsyncTask(taskId);
    return task && task->status.load() == AsyncLoadStatus::Loaded;
}

bool SceneManager::IsSceneLoaded(const std::string& path) const
{
    auto task = FindAsyncTask(path);
    return task && task->status.load() == AsyncLoadStatus::Loaded;
}

bool SceneManager::IsSceneLoaded(const AssetUUID& sceneAsset) const
{
    auto task = FindAsyncTask(sceneAsset);
    return task && task->status.load() == AsyncLoadStatus::Loaded;
}

float SceneManager::GetLoadProgress(uint32_t taskId) const
{
    auto task = FindAsyncTask(taskId);
    return task ? task->progress.load() : 0.0f;
}

float SceneManager::GetLoadProgress(const std::string& path) const
{
    auto task = FindAsyncTask(path);
    return task ? task->progress.load() : 0.0f;
}

float SceneManager::GetLoadProgress(const AssetUUID& sceneAsset) const
{
    auto task = FindAsyncTask(sceneAsset);
    return task ? task->progress.load() : 0.0f;
}

SceneManager::AsyncLoadStatus SceneManager::GetLoadStatus(uint32_t taskId) const
{
    auto task = FindAsyncTask(taskId);
    return task ? task->status.load() : AsyncLoadStatus::None;
}

SceneManager::AsyncLoadStatus SceneManager::GetLoadStatus(const std::string& path) const
{
    auto task = FindAsyncTask(path);
    return task ? task->status.load() : AsyncLoadStatus::None;
}

SceneManager::AsyncLoadStatus SceneManager::GetLoadStatus(const AssetUUID& sceneAsset) const
{
    auto task = FindAsyncTask(sceneAsset);
    return task ? task->status.load() : AsyncLoadStatus::None;
}

bool SceneManager::ChangeToLoadedScene(uint32_t taskId)
{
    auto task = FindAsyncTask(taskId);
    if (!task) return false;

    if (task->status.load() != AsyncLoadStatus::Loaded)
    {
        if (task->jsonParsed.load() && task->scene == nullptr)
        {
            task->progress.store(0.9f);
            auto scenePtr = CreateScene(task->sceneName);
            try
            {
                m_SceneSerializer.DeserializeScene(*scenePtr, task->parsedJson);
                scenePtr->SetPath(task->path);
                task->scene = std::move(scenePtr);
                task->parsedJson.clear();
                task->progress.store(1.0f);
                task->status.store(AsyncLoadStatus::Loaded);
            }
            catch (const std::exception& e)
            {
                task->errorMessage = std::string("Scene deserialization failed: ") + e.what();
                task->status.store(AsyncLoadStatus::Failed);
                LOG_ERROR("{}", task->errorMessage);
                return false;
            }
        }
        else
        {
            LOG_WARN("Cannot change to scene '{}': still loading (status: {})", task->path, static_cast<int>(task->status.load()));
            return false;
        }
    }

    m_PendingLoadedTaskId = task->id;
    m_HasPendingLoadedScene = true;
    return true;
}

bool SceneManager::ChangeToLoadedScene(const std::string& path)
{
    auto task = FindAsyncTask(path);
    if (!task) return false;
    return ChangeToLoadedScene(task->id);
}

bool SceneManager::ChangeToLoadedScene(const AssetUUID& sceneAsset)
{
    auto task = FindAsyncTask(sceneAsset);
    if (!task) return false;
    return ChangeToLoadedScene(task->id);
}

bool SceneManager::CancelAsyncLoad(uint32_t taskId)
{
    std::shared_ptr<AsyncSceneLoadTask> taskToCancel = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        for (auto it = m_AsyncTasks.begin(); it != m_AsyncTasks.end(); ++it)
        {
            if ((*it)->id == taskId)
            {
                taskToCancel = *it;
                taskToCancel->cancelled.store(true);
                m_AsyncTasks.erase(it);
                break;
            }
        }
    }

    if (taskToCancel)
    {
        if (taskToCancel->workerThread.joinable())
            taskToCancel->workerThread.join();
        return true;
    }
    return false;
}

bool SceneManager::CancelAsyncLoad(const std::string& path)
{
    auto task = FindAsyncTask(path);
    if (!task) return false;
    return CancelAsyncLoad(task->id);
}

bool SceneManager::CancelAsyncLoad(const AssetUUID& sceneAsset)
{
    auto task = FindAsyncTask(sceneAsset);
    if (!task) return false;
    return CancelAsyncLoad(task->id);
}

bool SceneManager::UnloadLoadedScene(uint32_t taskId)
{
    return CancelAsyncLoad(taskId);
}

bool SceneManager::UnloadLoadedScene(const std::string& path)
{
    return CancelAsyncLoad(path);
}

bool SceneManager::UnloadLoadedScene(const AssetUUID& sceneAsset)
{
    return CancelAsyncLoad(sceneAsset);
}

std::vector<std::string> SceneManager::GetLoadedScenePaths() const
{
    std::vector<std::string> result;
    std::lock_guard<std::mutex> lock(m_AsyncMutex);
    for (const auto& task : m_AsyncTasks)
    {
        if (task->status.load() == AsyncLoadStatus::Loaded)
            result.push_back(task->path);
    }
    return result;
}

void SceneManager::ClearAsyncLoads()
{
    std::vector<std::shared_ptr<AsyncSceneLoadTask>> tasksToJoin;
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        for (auto& task : m_AsyncTasks)
        {
            task->cancelled.store(true);
        }
        tasksToJoin = std::move(m_AsyncTasks);
        m_AsyncTasks.clear();
    }

    for (auto& task : tasksToJoin)
    {
        if (task->workerThread.joinable())
        {
            task->workerThread.join();
        }
    }
}

void SceneManager::ActivateSceneInternal(std::unique_ptr<Scene> scenePtr)
{
    if (!scenePtr) return;

    const SceneState previousState = m_CurrentScene ? m_CurrentScene->GetState() : SceneState::EDIT;
    if (m_CurrentScene && (previousState == SceneState::PLAY || previousState == SceneState::PAUSE))
    {
        CaptureAutoloadStates(*m_CurrentScene);
    }
    if (m_CurrentScene && previousState != SceneState::EDIT)
    {
        m_CurrentScene->SetChangingScene(true);
        m_CurrentScene->SetState(SceneState::EDIT);
    }

    SetActiveScene(std::move(scenePtr));
    if (previousState == SceneState::PLAY)
    {
        m_CurrentScene->SetState(SceneState::PLAY);
    }
    else if (previousState == SceneState::PAUSE)
    {
        m_CurrentScene->SetState(SceneState::PLAY);
        m_CurrentScene->SetState(SceneState::PAUSE);
    }
}

void SceneManager::ActivateLoadedSceneInternal(uint32_t taskId)
{
    std::unique_ptr<Scene> sceneToActivate = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        for (auto it = m_AsyncTasks.begin(); it != m_AsyncTasks.end(); ++it)
        {
            if ((*it)->id == taskId)
            {
                sceneToActivate = std::move((*it)->scene);
                if ((*it)->workerThread.joinable())
                    (*it)->workerThread.join();
                m_AsyncTasks.erase(it);
                break;
            }
        }
    }

    if (sceneToActivate)
    {
        ActivateSceneInternal(std::move(sceneToActivate));
    }
}

void SceneManager::ProcessAsyncLoads()
{
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        for (auto& task : m_AsyncTasks)
        {
            if (task->workerThread.joinable() && (task->jsonParsed.load() || task->status.load() == AsyncLoadStatus::Failed || task->status.load() == AsyncLoadStatus::Cancelled))
            {
                task->workerThread.join();
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        for (auto& task : m_AsyncTasks)
        {
            if (task->cancelled.load())
                continue;

            if (task->status.load() == AsyncLoadStatus::Loading && task->jsonParsed.load() && task->scene == nullptr)
            {
                task->progress.store(0.9f);
                auto scenePtr = CreateScene(task->sceneName);
                try
                {
                    m_SceneSerializer.DeserializeScene(*scenePtr, task->parsedJson);
                    scenePtr->SetPath(task->path);
                    task->scene = std::move(scenePtr);
                    task->parsedJson.clear();
                    task->progress.store(1.0f);
                    task->status.store(AsyncLoadStatus::Loaded);
                }
                catch (const std::exception& e)
                {
                    task->errorMessage = std::string("Scene deserialization failed: ") + e.what();
                    task->status.store(AsyncLoadStatus::Failed);
                    LOG_ERROR("{}", task->errorMessage);
                }
                break;
            }
        }
    }

    uint32_t autoActivateTaskId = 0;
    {
        std::lock_guard<std::mutex> lock(m_AsyncMutex);
        for (const auto& task : m_AsyncTasks)
        {
            if (task->autoActivate && task->status.load() == AsyncLoadStatus::Loaded && task->scene != nullptr)
            {
                autoActivateTaskId = task->id;
                break;
            }
        }
    }

    if (autoActivateTaskId != 0)
    {
        ActivateLoadedSceneInternal(autoActivateTaskId);
    }
}

void SceneManager::Update()
{
    if (m_CurrentScene)
        m_CurrentScene->Update();

    ProcessAsyncLoads();

    if (m_HasPendingLoadedScene)
    {
        uint32_t taskId = m_PendingLoadedTaskId;
        m_HasPendingLoadedScene = false;
        m_PendingLoadedTaskId = 0;
        ActivateLoadedSceneInternal(taskId);
    }
    else if (m_HasPendingScene)
    {
		const std::string pendingPath = std::move(m_PendingScenePath);
        m_HasPendingScene = false;
		m_PendingScenePath.clear();

		const std::filesystem::path path(pendingPath);
		std::unique_ptr<Scene> scenePtr = CreateScene(path.stem().string());
		if (!m_SceneSerializer.LoadScene(*scenePtr, pendingPath))
			return;

		ActivateSceneInternal(std::move(scenePtr));
    }
}

void SceneManager::CaptureAutoloadStates(Scene& scene)
{
	m_AutoloadStates.clear();
	auto view = scene.GetRegistry().view<AutoloadComponent>();
	for (auto e : view)
	{
		const auto& ac = view.get<AutoloadComponent>(e);
		json state = json::object();
		if (scene.HasComponent<ScriptComponent>(e))
		{
			auto& sc = scene.GetComponent<ScriptComponent>(e);
			if (sc.luaInstance.valid())
			{
				state["lua"] = ScriptUtils::SerializeLuaInstanceData(sc.luaInstance);
			}
		}
		if (scene.HasComponent<TransformComponent>(e))
		{
			const auto& tc = scene.GetComponent<TransformComponent>(e);
			const auto& pos = tc.GetPosition();
			const auto& rot = tc.GetEulerRotation();
			const auto& scl = tc.GetScale();
			state["transform"] = {
				{ "position", { pos.x, pos.y, pos.z } },
				{ "rotation", { rot.x, rot.y, rot.z } },
				{ "scale", { scl.x, scl.y, scl.z } }
			};
		}
		m_AutoloadStates[ac.name] = state;
	}
}

static void MarkAutoloadDescendantsReady(Scene& scene, entt::entity parent)
{
	if (!scene.HasComponent<SceneTreeComponent>(parent)) return;
	const auto& children = scene.GetComponent<SceneTreeComponent>(parent).children;
	for (entt::entity child : children)
	{
		if (scene.HasComponent<ScriptComponent>(child))
		{
			auto& sc = scene.GetComponent<ScriptComponent>(child);
			if (!sc.luaInstance.valid())
				scene.GetScriptSystem().BindLuaScript(sc, child);
			sc.runtimeInitialized = true;
			sc.runtimeEnabled = true;
			sc.runtimeReady = true;
		}
		MarkAutoloadDescendantsReady(scene, child);
	}
}

void SceneManager::RestoreAutoloadStates(Scene& scene)
{
	if (m_AutoloadStates.empty()) return;
	auto view = scene.GetRegistry().view<AutoloadComponent>();
	for (auto e : view)
	{
		const auto& ac = view.get<AutoloadComponent>(e);
		auto it = m_AutoloadStates.find(ac.name);
		if (it == m_AutoloadStates.end()) continue;

		const json& state = it->second;
		if (state.contains("transform") && scene.HasComponent<TransformComponent>(e))
		{
			auto& tc = scene.GetComponent<TransformComponent>(e);
			const auto& t = state["transform"];
			if (t.contains("position")) tc.SetPosition(glm::vec3(t["position"][0], t["position"][1], t["position"][2]));
			if (t.contains("rotation")) tc.SetEulerRotation(glm::vec3(t["rotation"][0], t["rotation"][1], t["rotation"][2]));
			if (t.contains("scale")) tc.SetScale(glm::vec3(t["scale"][0], t["scale"][1], t["scale"][2]));
		}

		if (scene.HasComponent<ScriptComponent>(e))
		{
			auto& sc = scene.GetComponent<ScriptComponent>(e);
			if (!sc.luaInstance.valid())
			{
				scene.GetScriptSystem().BindLuaScript(sc, e);
			}

			if (state.contains("lua") && sc.luaInstance.valid())
			{
				ScriptUtils::DeserializeLuaInstanceData(state["lua"], sc.luaInstance);
			}

			sc.runtimeInitialized = true;
			sc.runtimeEnabled = true;
			sc.runtimeReady = true;

			sol::state& lua = scene.GetScriptSystem().GetLuaState();
			if (sc.luaInstance.valid())
				lua[ac.name] = sc.luaInstance;
			else
				lua[ac.name] = Entity(e, &scene);
		}
		else
		{
			sol::state& lua = scene.GetScriptSystem().GetLuaState();
			lua[ac.name] = Entity(e, &scene);
		}

		MarkAutoloadDescendantsReady(scene, e);
	}
}

void SceneManager::FixedUpdate()
{
    GetActiveScene().FixedUpdate();
}

void SceneManager::LateUpdate()
{
    GetActiveScene().LateUpdate();
}
