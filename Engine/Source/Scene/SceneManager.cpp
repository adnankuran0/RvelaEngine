#include "rvelapch.h"
#include "SceneManager.h"
#include "json.hpp"
#include "EntityUUID.h"  
#include "Core/Log.h"
#include <Utils/Serializer.h>
#include "Renderer/TextureCache.h"

using namespace rv;

using json = nlohmann::json;

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

void SceneManager::LoadScene(const std::string& path)
{
    m_PendingScenePath = path;
    m_HasPendingScene = true;
}

void SceneManager::Update()
{
    GetActiveScene().Update();

    if (m_HasPendingScene)
    {
        if (m_CurrentScene && m_CurrentScene->GetState() != SceneState::EDIT)
        {
            StopScene();
        }
        std::unique_ptr<Scene> scenePtr = CreateScene("NewScene");
        m_SceneSerializer.LoadScene(*scenePtr, m_PendingScenePath);
        SetActiveScene(std::move(scenePtr));
        m_HasPendingScene = false;
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
