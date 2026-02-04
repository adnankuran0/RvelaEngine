#include "rvelapch.h"
#include "SceneManager.h"
#include "json.hpp"
#include "EntityUUID.h"  
#include "Core/Log.h"
#include <Utils/Serializer.h>

using json = nlohmann::json;

void SceneManager::SaveScene(const std::string& path)
{
    m_SceneSerializer.SaveScene(GetActiveScene(), path);
}

void SceneManager::LoadScene(const std::string& path)
{
    std::unique_ptr<Scene> scenePtr = CreateScene("NewScene");
    m_SceneSerializer.LoadScene(*scenePtr, path);
    SetActiveScene(std::move(scenePtr));
}


