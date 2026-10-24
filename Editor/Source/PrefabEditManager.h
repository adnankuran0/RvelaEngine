#pragma once

#include "Asset/AssetUUID.h"
#include "Scene/EntityUUID.h"
#include <entt/entt.h>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <glm/glm.hpp>

namespace rv {

class Engine;
class Scene;

struct SavedEditorCameraState
{
    glm::vec3 position{ 0.0f, 2.0f, 5.0f };
    glm::vec3 targetPosition{ 0.0f, 2.0f, 5.0f };
    glm::vec3 orbitTarget{ 0.0f, 0.0f, 0.0f };
    float orbitDistance{ 5.0f };
    float yaw{ -90.0f };
    float pitch{ 0.0f };
    glm::vec3 front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right{ 1.0f, 0.0f, 0.0f };
};

class PrefabEditManager
{
public:
    static PrefabEditManager& Get();

    bool IsInPrefabMode() const { return m_IsInPrefabMode; }
    const AssetUUID& GetCurrentPrefabUUID() const { return m_CurrentPrefabUUID; }
    const std::string& GetPrefabName() const { return m_PrefabName; }
    const std::filesystem::path& GetPrefabPath() const { return m_PrefabPath; }
    entt::entity GetPrefabRootEntity() const { return m_PrefabRootEntity; }
    entt::entity GetAuxiliaryLightEntity() const { return m_AuxiliaryLightEntity; }

    bool OpenPrefab(Engine& engine, const AssetUUID& prefabUUID);
    bool SaveCurrentPrefab(Engine& engine);
    void ExitPrefabMode(Engine& engine, bool save = true);

private:
    PrefabEditManager() = default;

    bool m_IsInPrefabMode = false;
    AssetUUID m_CurrentPrefabUUID = AssetUUID::Invalid();
    std::string m_PrefabName;
    std::filesystem::path m_PrefabPath;
    entt::entity m_PrefabRootEntity = entt::null;
    entt::entity m_AuxiliaryLightEntity = entt::null;

    std::unique_ptr<Scene> m_SavedMainScene = nullptr;

    SavedEditorCameraState m_SavedMainSceneCamera;
    bool m_HasSavedCamState = false;

    std::unordered_map<AssetUUID, SavedEditorCameraState> m_PrefabCameraStates;

    std::vector<EntityUUID> m_SavedSelectedUUIDs;
};

} // namespace rv
