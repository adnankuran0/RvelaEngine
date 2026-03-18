#include "rvelapch.h"
#include "SceneManager.h"
#include "json.hpp"
#include "EntityUUID.h"  
#include "Core/Log.h"
#include <Utils/Serializer.h>
#include "Rendering/TextureCache.h"

using namespace rv;

using json = nlohmann::json;

void SceneManager::Init()
{
    SetActiveScene(CreateScene("EmptyScene"));
}

void SceneManager::SaveScene(const std::string& path)
{
    
    m_SceneSerializer.SaveScene(GetActiveScene(), path);
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
