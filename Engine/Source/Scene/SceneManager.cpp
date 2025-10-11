#include "rvelapch.h"
#include "SceneManager.h"
#include "json.hpp"
#include "EntityUUID.h"  
#include "Core/Log.h"
#include <Utils/Serializer.h>

using json = nlohmann::json;

Scene SceneManager::CreateScene(const std::string& path)
{
    Scene newScene;
    //TODO: set scenes self path
    m_CurrentScene = &newScene;
    return newScene;
}

Scene SceneManager::CreateScene()
{
    Scene newScene;
    m_CurrentScene = &newScene;
    return newScene;
}

void SceneManager::SaveScene(Scene& scene, const std::string& path)
{
    
}

void SceneManager::LoadScene(Scene& scene, const std::string& path)
{
    
}

unsigned int SceneManager::CountEntitiesRecursively(entt::entity& rootEntity)
{
    unsigned int count = 0;
    if (m_CurrentScene->HasComponent<SceneTreeComponent>(rootEntity))
    {
        auto& children = m_CurrentScene->GetComponent<SceneTreeComponent>(rootEntity).children;
        for (auto child : children)
            count += CountEntitiesRecursively(child);
    }
    
    return count;
}

void SceneManager::SerializeEntityRecursively(entt::entity& e, Scene& scene, std::vector<std::byte>& buffer, json& j)
{
    // serialize component count
    unsigned int componentCount = 0; 
    for (auto& rootEntity : m_CurrentScene->GetRootEntities())
    {
        componentCount += scene.GetComponentCount(e);
    }

    const std::byte* data = reinterpret_cast<const std::byte*>(&componentCount);
    buffer.insert(buffer.end(), data, data + sizeof(unsigned int));

    // serialize all components
    auto& uuidComp = scene.GetComponent<UUIDComponent>(e);
    SerializeBin_UUIDComp(uuidComp, buffer);

    if (scene.HasComponent<TagComponent>(e))
    {
        auto& comp = scene.GetComponent<TagComponent>(e);
        SerializeBin_TagComp(comp, buffer);
    }

    if (scene.HasComponent<TransformComponent>(e))
    {
        auto& comp = scene.GetComponent<TransformComponent>(e);
        SerializeBin_TransformComp(comp, buffer);
    }

    if (scene.HasComponent<MaterialComponent>(e))
    {
        auto& comp = scene.GetComponent<MaterialComponent>(e);
        comp.GetMaterial()->Serialize();
        SerializeBin_MaterialComp(comp, buffer);
    }

    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& comp = scene.GetComponent<SceneTreeComponent>(e);
        SerializeBin_SceneTreeComp(comp, buffer);
    }

    if (scene.HasComponent<MeshComponent>(e))
    {
        auto& comp = scene.GetComponent<MeshComponent>(e);
        SerializeBin_MeshComp(comp, buffer);
    }

    if (scene.HasComponent<MeshRendererComponent>(e))
    {
        auto& comp = scene.GetComponent<MeshRendererComponent>(e);

        SerializeBin_MeshRendererComp(comp, buffer);
    }

    if (scene.HasComponent<PointLightComponent>(e))
    {
        auto& comp = scene.GetComponent<PointLightComponent>(e);
        SerializeBin_PointLightComp(comp, buffer);
    }

    if (scene.HasComponent<DirectionalLightComponent>(e))
    {
        auto& comp = scene.GetComponent<DirectionalLightComponent>(e);
        SerializeBin_DirectionalLightComp(comp, buffer);
    }

    if (scene.HasComponent<SceneTreeComponent>(e))
    {
        auto& children = scene.GetComponent<SceneTreeComponent>(e).children;
        if (children.empty()) return;

        for (auto& child : children)
        {
            SerializeEntityRecursively(child, scene, buffer, j);
        }
    }

}