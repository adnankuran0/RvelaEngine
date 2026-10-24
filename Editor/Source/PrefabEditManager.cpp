#include "PrefabEditManager.h"
#include "Core/Engine.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/SceneManager.h"
#include "Scene/Components/TagComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/PrefabComponent.h"
#include "Scene/Components/DirectionalLightComponent.h"
#include "Scene/Components/PointLightComponent.h"
#include "Scene/Components/SpotLightComponent.h"
#include "Asset/AssetManager.h"
#include "AssetImporters/PrefabImporter.h"
#include "EditorSelection.h"
#include "Commands/CommandHistory.h"
#include "Core/Log.h"
#include "Renderer/EditorCamera.h"
#include "Utils/FileUtils.h"

namespace rv {

PrefabEditManager& PrefabEditManager::Get()
{
    static PrefabEditManager s_Instance;
    return s_Instance;
}

bool PrefabEditManager::OpenPrefab(Engine& engine, const AssetUUID& prefabUUID)
{
    if (!prefabUUID.IsValid())
        return false;

    if (m_IsInPrefabMode)
    {
        if (m_CurrentPrefabUUID == prefabUUID)
            return true;

        SaveCurrentPrefab(engine);
        ExitPrefabMode(engine, false);
    }

    if (engine.GetActiveScene().GetState() == SceneState::PLAY)
    {
        LOG_WARN("[PrefabEditManager] Cannot open Prefab Mode while the scene is playing.");
        return false;
    }

    AssetRegistry& reg = AssetManager::Get().GetRegistry();
    std::filesystem::path path = reg.GetPath(prefabUUID);
    if (path.empty() || !std::filesystem::exists(path))
    {
        LOG_ERROR("[PrefabEditManager] Prefab file not found for UUID: {}", prefabUUID.ToString());
        return false;
    }

    m_CurrentPrefabUUID = prefabUUID;
    m_PrefabPath = path;
    m_PrefabName = path.stem().string();

    if (auto* cam = engine.GetEditorCamera())
    {
        m_SavedMainSceneCamera.position = cam->Position;
        m_SavedMainSceneCamera.targetPosition = cam->targetPosition;
        m_SavedMainSceneCamera.orbitTarget = cam->orbitTarget;
        m_SavedMainSceneCamera.orbitDistance = cam->orbitDistance;
        m_SavedMainSceneCamera.yaw = cam->Yaw;
        m_SavedMainSceneCamera.pitch = cam->Pitch;
        m_SavedMainSceneCamera.front = cam->Front;
        m_SavedMainSceneCamera.up = cam->Up;
        m_SavedMainSceneCamera.right = cam->Right;
        m_HasSavedCamState = true;

        auto it = m_PrefabCameraStates.find(m_CurrentPrefabUUID);
        if (it != m_PrefabCameraStates.end())
        {
            cam->Position = it->second.position;
            cam->targetPosition = it->second.targetPosition;
            cam->orbitTarget = it->second.orbitTarget;
            cam->orbitDistance = it->second.orbitDistance;
            cam->Yaw = it->second.yaw;
            cam->Pitch = it->second.pitch;
            cam->Front = it->second.front;
            cam->Up = it->second.up;
            cam->Right = it->second.right;
            cam->ResetMotion();
        }
        else
        {
            cam->Focus(glm::vec3(0.0f, 0.5f, 0.0f), 4.0f);
        }
    }

    m_SavedSelectedUUIDs.clear();
    for (auto e : EditorSelection::Get().GetSelectedEntities())
    {
        if (engine.GetActiveScene().GetRegistry().valid(e) && engine.GetActiveScene().HasComponent<UUIDComponent>(e))
        {
            m_SavedSelectedUUIDs.push_back(engine.GetActiveScene().GetComponent<UUIDComponent>(e).uuid);
        }
    }
    EditorSelection::Get().Clear();
    CommandHistory::Get().Clear();

    auto prefabScene = engine.GetSceneManager().CreateScene("Prefab: " + m_PrefabName);
    prefabScene->GetEnvironment().CopySettingsFrom(engine.GetActiveScene().GetEnvironment());

    Path editorHdr = EDITOR_PATH("environment.hdr");
    if (!std::filesystem::exists(editorHdr.GetAbsolute()))
    {
        editorHdr = EDITOR_PATH("TemplateProject/Assets/Textures/skybox/environment.hdr");
    }
    if (std::filesystem::exists(editorHdr.GetAbsolute()))
    {
        prefabScene->GetEnvironment().GetSkybox().InitHDR(editorHdr);
    }

    m_SavedMainScene = engine.GetSceneManager().SwapActiveScene(std::move(prefabScene));

    Scene& activeScene = engine.GetActiveScene();
    Entity root = activeScene.Instantiate(m_CurrentPrefabUUID, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), activeScene.GetRootEntity());
    if (root.GetHandle() == entt::null)
    {
        LOG_ERROR("[PrefabEditManager] Failed to instantiate prefab '{}' for editing.", m_PrefabName);
        ExitPrefabMode(engine, false);
        return false;
    }

    m_PrefabRootEntity = root.GetHandle();

    // this entity IS the prefab root being authored  so remove the instance wrapper
    if (activeScene.HasComponent<PrefabComponent>(m_PrefabRootEntity))
    {
        activeScene.RemoveComponent<PrefabComponent>(m_PrefabRootEntity);
    }

    bool hasLight = false;
    auto& registry = activeScene.GetRegistry();
    if (!registry.view<DirectionalLightComponent>().empty() ||
        !registry.view<PointLightComponent>().empty() ||
        !registry.view<SpotLightComponent>().empty())
    {
        hasLight = true;
    }

    if (!hasLight)
    {
        Entity editorLight = activeScene.CreateEntity("__PrefabEditorLight__");
        editorLight.GetComponent<TransformComponent>().SetEulerRotation(glm::vec3(-50.0f, -40.0f, 0.0f));
        auto& dlc = editorLight.AddComponent<DirectionalLightComponent>();
        dlc.intensity = 3.0f;
        dlc.color = glm::vec3(1.0f);
        m_AuxiliaryLightEntity = editorLight.GetHandle();
    }
    else
    {
        m_AuxiliaryLightEntity = entt::null;
    }

    EditorSelection::Get().Select(m_PrefabRootEntity);

    m_IsInPrefabMode = true;
    activeScene.SetDirty(false);

    return true;
}

bool PrefabEditManager::SaveCurrentPrefab(Engine& engine)
{
    if (!m_IsInPrefabMode || !m_CurrentPrefabUUID.IsValid() || m_PrefabRootEntity == entt::null)
        return false;

    Scene& activeScene = engine.GetActiveScene();
    if (!activeScene.GetRegistry().valid(m_PrefabRootEntity))
    {
        LOG_ERROR("[PrefabEditManager] Prefab root entity is invalid, cannot save!");
        return false;
    }

    Ref<PrefabAsset> asset = PrefabImporter::CreatePrefabAsset(m_PrefabPath, m_CurrentPrefabUUID, activeScene, m_PrefabRootEntity);
    if (!asset)
    {
        LOG_ERROR("[PrefabEditManager] Failed to save prefab file: {}", m_PrefabPath.string());
        return false;
    }

    activeScene.SetDirty(false);
    AssetManager::Get().Unload(m_CurrentPrefabUUID);

    if (m_SavedMainScene)
    {
        std::vector<entt::entity> instances;
        auto view = m_SavedMainScene->GetRegistry().view<PrefabComponent>();
        for (auto e : view)
        {
            if (view.get<PrefabComponent>(e).GetPrefabID() == m_CurrentPrefabUUID)
            {
                instances.push_back(e);
            }
        }
        for (auto inst : instances)
        {
            PrefabImporter::RevertPrefab(*m_SavedMainScene, inst, true);
        }
    }

    return true;
}

void PrefabEditManager::ExitPrefabMode(Engine& engine, bool save)
{
    if (!m_IsInPrefabMode)
        return;

    if (save)
    {
        SaveCurrentPrefab(engine);
    }

    EditorSelection::Get().Clear();
    CommandHistory::Get().Clear();

    if (auto* cam = engine.GetEditorCamera())
    {
        SavedEditorCameraState state;
        state.position = cam->Position;
        state.targetPosition = cam->targetPosition;
        state.orbitTarget = cam->orbitTarget;
        state.orbitDistance = cam->orbitDistance;
        state.yaw = cam->Yaw;
        state.pitch = cam->Pitch;
        state.front = cam->Front;
        state.up = cam->Up;
        state.right = cam->Right;
        m_PrefabCameraStates[m_CurrentPrefabUUID] = state;
    }

    if (m_SavedMainScene)
    {
        engine.GetSceneManager().SwapActiveScene(std::move(m_SavedMainScene));
    }
    else
    {
        engine.GetSceneManager().SetActiveScene(engine.GetSceneManager().CreateScene("Untitled"));
    }

    if (m_HasSavedCamState)
    {
        if (auto* cam = engine.GetEditorCamera())
        {
            cam->Position = m_SavedMainSceneCamera.position;
            cam->targetPosition = m_SavedMainSceneCamera.targetPosition;
            cam->orbitTarget = m_SavedMainSceneCamera.orbitTarget;
            cam->orbitDistance = m_SavedMainSceneCamera.orbitDistance;
            cam->Yaw = m_SavedMainSceneCamera.yaw;
            cam->Pitch = m_SavedMainSceneCamera.pitch;
            cam->Front = m_SavedMainSceneCamera.front;
            cam->Up = m_SavedMainSceneCamera.up;
            cam->Right = m_SavedMainSceneCamera.right;
            cam->ResetMotion();
        }
        m_HasSavedCamState = false;
    }

    EditorSelection::Get().Clear();
    for (auto uuid : m_SavedSelectedUUIDs)
    {
        auto it = engine.GetActiveScene().GetUUIDEntityMap().find(uuid);
        if (it != engine.GetActiveScene().GetUUIDEntityMap().end() && engine.GetActiveScene().GetRegistry().valid(it->second))
        {
            EditorSelection::Get().Add(it->second);
        }
    }
    if (!EditorSelection::Get().IsEmpty())
    {
        EditorSelection::Get().SetPrimary(EditorSelection::Get().GetSelectedEntities().front());
    }
    m_SavedSelectedUUIDs.clear();

    m_IsInPrefabMode = false;
    m_CurrentPrefabUUID = AssetUUID::Invalid();
    m_PrefabPath.clear();
    m_PrefabName.clear();
    m_PrefabRootEntity = entt::null;
    m_AuxiliaryLightEntity = entt::null;

    EditorSelection::Get().Validate(engine.GetActiveScene().GetRegistry());
}

} // namespace rv
