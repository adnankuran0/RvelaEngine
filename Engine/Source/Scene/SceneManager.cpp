#include "rvelapch.h"
#include "SceneManager.h"
#include "json.hpp"
#include "EntityUUID.h"  
#include "Core/Log.h"
#include <Utils/Serializer.h>
#include "Renderer/TextureCache.h"
#include "Asset/AssetManager.h"
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
    if (!m_CurrentScene) return;

    if (m_CurrentScene->GetState() == SceneState::PLAY || m_CurrentScene->GetState() == SceneState::PAUSE)
    {
        m_CurrentScene->SetState(SceneState::EDIT);

        if (m_HasSnapshot)
        {
            std::unique_ptr<Scene> restoredScene = CreateScene(m_SnapshotSceneName.empty() ? "Untitled" : m_SnapshotSceneName);
            restoredScene->SetPath(m_SnapshotScenePath);
            m_SceneSerializer.DeserializeScene(*restoredScene, m_SceneSnapshot);
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
    if (!ResolveSceneFile(path, resolved))
        return false;

    m_PendingScenePath = resolved.string();
    m_HasPendingScene = true;
    return true;
}

bool SceneManager::LoadScene(const AssetUUID& sceneAsset)
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

	return LoadScene(path.string());
}

void SceneManager::Update()
{
    if (m_CurrentScene)
        m_CurrentScene->Update();

    if (m_HasPendingScene)
    {
		const std::string pendingPath = std::move(m_PendingScenePath);
        m_HasPendingScene = false;
		m_PendingScenePath.clear();

		const std::filesystem::path path(pendingPath);
		std::unique_ptr<Scene> scenePtr = CreateScene(path.stem().string());
		if (!m_SceneSerializer.LoadScene(*scenePtr, pendingPath))
			return;

		const SceneState previousState = m_CurrentScene ? m_CurrentScene->GetState() : SceneState::EDIT;
		if (m_CurrentScene && previousState != SceneState::EDIT)
			m_CurrentScene->SetState(SceneState::EDIT);

		SetActiveScene(std::move(scenePtr));
		if (previousState == SceneState::PLAY)
			m_CurrentScene->SetState(SceneState::PLAY);
		else if (previousState == SceneState::PAUSE)
		{
			m_CurrentScene->SetState(SceneState::PLAY);
			m_CurrentScene->SetState(SceneState::PAUSE);
		}
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
