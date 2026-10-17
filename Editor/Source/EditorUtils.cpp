#include "EditorUtils.h"
#include "GUI/Dialogs.h"
#include "GUI/EditorFileDialog.h"
#include <fstream>
#include "Core/Engine.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetRegistry.h"
#include "Utils/ProjectManager.h"
#include <cstdlib>
#include <filesystem>
#include "EditorSelection.h"
#include "Commands/CommandHistory.h"
#include "Commands/TransformCommand.h"
#include "Renderer/EditorCamera.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Components/SceneTreeComponent.h"
#include "Scene/Components/UUIDComponent.h"
#include <glm/gtc/quaternion.hpp>

using namespace rv;

void EditorUtils::OpenVSCodeWorkspace()
{
    if (!ProjectManager::IsProjectLoaded())
        return;

    const auto scriptsDir = ProjectManager::GetAssetDirectory() / "Scripts";
    if (!std::filesystem::exists(scriptsDir))
        return;

    const std::string command = "code \"" + scriptsDir.string() + "\"";
    std::system(command.c_str());
}

void EditorUtils::CreateScene(Engine& engine)
{
    CommandHistory::Get().Clear();
    EditorSelection::Get().Clear();
    auto& sm = engine.GetSceneManager();
    sm.SetActiveScene(sm.CreateScene("NewScene"));
    sm.GetActiveScene().SetDirty(false);
}

void EditorUtils::OpenScene(Engine& engine)
{
    std::filesystem::path startDir = ProjectManager::IsProjectLoaded()
        ? ProjectManager::GetAssetDirectory()
        : std::filesystem::current_path();

    if (std::filesystem::exists(startDir / "Scenes"))
        startDir /= "Scenes";

    EditorFileDialog::Open(
        "Open Scene",
        FileDialogMode::OpenFile,
        { ".rscene" },
        startDir,
        [&engine](const std::filesystem::path& chosen) {
            CommandHistory::Get().Clear();
            EditorSelection::Get().Clear();
            engine.GetSceneManager().LoadScene(chosen.string());
        }
    );
}

bool EditorUtils::SaveScene(Engine& engine)
{
    Scene& activeScene = engine.GetActiveScene();
    if (activeScene.GetState() != SceneState::EDIT)
    {
        LOG_WARN("Scene can only be saved in EDIT mode.");
        return false;
    }
    if (!activeScene.GetPath().empty())
    {
        engine.GetSceneManager().SaveScene(activeScene.GetPath());
        activeScene.SetDirty(false);
        return true;
    }
    else
    {
        return SaveSceneAs(engine);
    }
}

bool EditorUtils::SaveSceneAs(Engine& engine)
{
    Scene& activeScene = engine.GetActiveScene();
    if (activeScene.GetState() != SceneState::EDIT)
    {
        LOG_WARN("Scene can only be saved in EDIT mode.");
        return false;
    }

    std::filesystem::path startDir = ProjectManager::IsProjectLoaded()
        ? ProjectManager::GetAssetDirectory()
        : std::filesystem::current_path();

    if (std::filesystem::exists(startDir / "Scenes"))
        startDir /= "Scenes";

    std::string defaultName = activeScene.GetName().empty() ? "NewScene.rscene" : (activeScene.GetName() + ".rscene");
    if (!defaultName.ends_with(".rscene"))
        defaultName += ".rscene";

    EditorFileDialog::Open(
        "Save Scene As",
        FileDialogMode::SaveFile,
        { ".rscene" },
        startDir,
        [&engine](const std::filesystem::path& chosen) {
            std::string file = chosen.string();
            std::ofstream ofs(file);
            if (ofs.is_open())
            {
                ofs.close();
                Scene& scene = engine.GetActiveScene();
                scene.SetName(chosen.stem().string());
                engine.GetSceneManager().SaveScene(file);
                scene.SetDirty(false);
            }
        },
        defaultName
    );
    return true;
}

static bool IsValidUUIDFormat(const std::string& str)
{
    if (str.length() != 36) return false;
    for (size_t i = 0; i < 36; ++i)
    {
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            if (str[i] != '-') return false;
        }
        else
        {
            if (!std::isxdigit(static_cast<unsigned char>(str[i]))) return false;
        }
    }
    return true;
}

AssetUUID EditorUtils::ReadUUIDFromMeta(const std::string& assetPath)
{
    if (assetPath.empty())
        return AssetUUID::Invalid();

    auto hashPos = assetPath.find('#');
    if (hashPos != std::string::npos)
    {
        std::string subUuidStr = assetPath.substr(hashPos + 1);
        if (IsValidUUIDFormat(subUuidStr))
        {
            AssetUUID subUUID = AssetUUID::FromString(subUuidStr);
            if (subUUID.IsValid())
                return subUUID;
        }
    }

    std::filesystem::path p(assetPath);
    std::string stemStr = p.stem().string();
    if (IsValidUUIDFormat(stemStr))
    {
        AssetUUID directUUID = AssetUUID::FromString(stemStr);
        if (directUUID.IsValid())
            return directUUID;
    }

    auto& reg = AssetManager::Get().GetRegistry();
    AssetUUID regUUID = reg.GetUUID(p);
    if (regUUID.IsValid())
        return regUUID;

    auto metaPath = AssetMeta::GetMetaPath(p);
    if (std::filesystem::exists(metaPath))
    {
        AssetMeta meta;
        if (meta.LoadFromFile(metaPath) && meta.uuid.IsValid())
        {
            reg.RegisterPath(meta.uuid, p);
            return meta.uuid;
        }
    }

    if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p))
    {
        AssetMeta meta = reg.GetOrCreateMeta(p);
        if (meta.uuid.IsValid())
            return meta.uuid;
    }

    return AssetUUID::Invalid();
}

static tsl::robin_map<AssetUUID, std::string> s_FileNameCache;

void EditorUtils::ClearAssetFileNameCache()
{
    s_FileNameCache.clear();
}

std::string EditorUtils::GetAssetFileName(const AssetUUID& uuid)
{
    if (!uuid.IsValid())
        return "";

    auto it = s_FileNameCache.find(uuid);
    if (it != s_FileNameCache.end())
        return it->second;

    auto path = AssetManager::Get().GetRegistry().GetPath(uuid);
    if (!path.empty())
    {
        std::string stem = path.stem().string();
        if (path.string().find(".cache") == std::string::npos && !AssetUUID::FromString(stem).IsValid())
        {
            std::string filename = path.filename().string();
            s_FileNameCache[uuid] = filename;
            return filename;
        }
    }

    // Fallback if not found in memory, scan assetDir once and cache all found UUIDs so we never scan disk again
    const auto& assetDir = AssetManager::Get().GetRegistry().GetAssetDir();
    if (!assetDir.empty() && std::filesystem::exists(assetDir))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".rmeta")
            {
                if (entry.path().string().find(".cache") != std::string::npos)
                    continue;

                AssetMeta meta;
                if (meta.LoadFromFile(entry.path()))
                {
                    auto srcPath = entry.path();
                    srcPath.replace_extension("");
                    std::string fn = srcPath.filename().string();

                    s_FileNameCache[meta.uuid] = fn;
                    for (const auto& sub : meta.subAssets)
                    {
                        if (sub.uuid.IsValid())
                        {
                            std::string displayName = sub.name.empty() ? fn : (fn + " : " + sub.name);
                            s_FileNameCache[sub.uuid] = displayName;
                        }
                    }
                }
            }
        }
    }

    auto cachedIt = s_FileNameCache.find(uuid);
    if (cachedIt != s_FileNameCache.end())
        return cachedIt->second;

    // Cache empty string to avoid rescanning filesystem for nonexistent assets
    s_FileNameCache[uuid] = "";
    return "";
}

std::vector<EditorUtils::MeshAssetInfo> EditorUtils::GetAvailableMeshAssets(bool skeletal)
{
    std::vector<MeshAssetInfo> results;
    const auto& assetDir = AssetManager::Get().GetRegistry().GetAssetDir();
    if (assetDir.empty() || !std::filesystem::exists(assetDir))
        return results;

    std::string targetType = skeletal ? "SkeletalMesh" : "Mesh";
    std::string targetExt = skeletal ? ".rskmesh" : ".rmesh";

    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDir))
    {
        if (!entry.is_regular_file()) continue;
        if (entry.path().string().find(".cache") != std::string::npos) continue;

        if (entry.path().extension() == ".rmeta")
        {
            AssetMeta meta;
            if (meta.LoadFromFile(entry.path()))
            {
                auto srcPath = entry.path();
                srcPath.replace_extension("");
                std::string modelName = srcPath.filename().string();

                for (const auto& sub : meta.subAssets)
                {
                    if (sub.uuid.IsValid() && sub.type == targetType)
                    {
                        MeshAssetInfo info;
                        info.uuid = sub.uuid;
                        info.modelName = modelName;
                        info.meshName = sub.name.empty() ? modelName : sub.name;
                        info.fullName = modelName + " : " + info.meshName;
                        results.push_back(info);
                    }
                }
            }
        }
        else if (entry.path().extension() == targetExt)
        {
            AssetUUID uuid = ReadUUIDFromMeta(entry.path().string());
            if (uuid.IsValid())
            {
                std::string fn = entry.path().filename().string();
                MeshAssetInfo info;
                info.uuid = uuid;
                info.modelName = fn;
                info.meshName = fn;
                info.fullName = fn;
                results.push_back(info);
            }
        }
    }

    std::sort(results.begin(), results.end(), [](const MeshAssetInfo& a, const MeshAssetInfo& b) {
        return a.fullName < b.fullName;
    });

    return results;
}

std::vector<EditorUtils::MaterialAssetInfo> EditorUtils::GetAvailableMaterialAssets()
{
    std::vector<MaterialAssetInfo> results;
    const auto& assetDir = AssetManager::Get().GetRegistry().GetAssetDir();
    if (assetDir.empty() || !std::filesystem::exists(assetDir))
        return results;

    std::error_code ec;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetDir, ec))
    {
        if (ec || !entry.is_regular_file()) continue;
        if (entry.path().string().find(".cache") != std::string::npos) continue;

        if (entry.path().extension() == ".rmat")
        {
            AssetUUID uuid = ReadUUIDFromMeta(entry.path().string());
            if (uuid.IsValid())
            {
                MaterialAssetInfo info;
                info.uuid = uuid;
                info.name = entry.path().stem().string();
                auto rel = std::filesystem::relative(entry.path(), assetDir, ec);
                info.path = ec ? entry.path().filename().string() : rel.string();
                results.push_back(info);
            }
        }
    }

    std::sort(results.begin(), results.end(), [](const MaterialAssetInfo& a, const MaterialAssetInfo& b) {
        return a.name < b.name;
    });

    return results;
}

bool EditorUtils::HasTransformSelection(Engine& engine)
{
    if (EditorSelection::Get().IsEmpty())
        return false;

    Scene& scene = engine.GetActiveScene();
    if (scene.GetState() != SceneState::EDIT)
        return false;

    auto& reg = scene.GetRegistry();
    for (auto e : EditorSelection::Get().GetSelectedEntities())
    {
        if (e != entt::null && reg.valid(e) && reg.any_of<TransformComponent>(e) && e != scene.GetRootEntity())
            return true;
    }
    return false;
}

void EditorUtils::AlignSelectedWithView(Engine& engine)
{
    EditorCamera* cam = engine.GetEditorCamera();
    if (!cam)
        return;

    Scene& scene = engine.GetActiveScene();
    if (scene.GetState() != SceneState::EDIT)
        return;

    auto& reg = scene.GetRegistry();
    const auto& selectedEntities = EditorSelection::Get().GetSelectedEntities();
    if (selectedEntities.empty())
        return;

    auto isAncestorSelected = [&](entt::entity candidate) -> bool {
        entt::entity cur = candidate;
        while (cur != entt::null && reg.valid(cur) && scene.HasComponent<SceneTreeComponent>(cur))
        {
            entt::entity parent = scene.GetComponent<SceneTreeComponent>(cur).parent;
            if (parent == entt::null || parent == scene.GetRootEntity()) break;
            if (std::find(selectedEntities.begin(), selectedEntities.end(), parent) != selectedEntities.end())
                return true;
            cur = parent;
        }
        return false;
    };

    glm::vec3 camWorldPos = cam->Position;
    glm::mat3 camBasis(cam->Right, cam->Up, -cam->Front);
    glm::quat camWorldRot = glm::normalize(glm::quat_cast(camBasis));

    std::vector<EntityTransformRecord> beforeRecords;
    std::vector<EntityTransformRecord> afterRecords;

    for (auto e : selectedEntities)
    {
        if (e == entt::null || !reg.valid(e) || !reg.any_of<TransformComponent>(e))
            continue;

        if (e == scene.GetRootEntity())
            continue;

        if (isAncestorSelected(e))
            continue;

        auto& tc = reg.get<TransformComponent>(e);
        EntityUUID uuid = 0;
        if (reg.any_of<UUIDComponent>(e))
            uuid = reg.get<UUIDComponent>(e).uuid;

        EntityTransformRecord beforeRec;
        beforeRec.uuid = uuid;
        beforeRec.entity = e;
        beforeRec.position = tc.GetPosition();
        beforeRec.rotation = tc.GetRotation();
        beforeRec.scale = tc.GetScale();

        glm::mat4 parentWorld(1.0f);
        glm::quat parentWorldRot(1.0f, 0.0f, 0.0f, 0.0f);
        bool hasParent = false;
        if (scene.HasComponent<SceneTreeComponent>(e))
        {
            auto& node = reg.get<SceneTreeComponent>(e);
            if (node.parent != entt::null && reg.valid(node.parent) && node.parent != scene.GetRootEntity() && reg.any_of<TransformComponent>(node.parent))
            {
                auto& parentTc = reg.get<TransformComponent>(node.parent);
                parentWorld = parentTc.GetWorldMatrix();
                parentWorldRot = parentTc.GetWorldRotation();
                hasParent = true;
            }
        }

        if (!hasParent)
        {
            tc.SetPosition(camWorldPos);
            tc.SetRotation(camWorldRot);
        }
        else
        {
            glm::mat4 invParentWorld = glm::inverse(parentWorld);
            tc.SetPosition(glm::vec3(invParentWorld * glm::vec4(camWorldPos, 1.0f)));
            tc.SetRotation(glm::normalize(glm::inverse(parentWorldRot) * camWorldRot));
        }

        tc.SetDirty(true);

        EntityTransformRecord afterRec;
        afterRec.uuid = uuid;
        afterRec.entity = e;
        afterRec.position = tc.GetPosition();
        afterRec.rotation = tc.GetRotation();
        afterRec.scale = tc.GetScale();

        beforeRecords.push_back(beforeRec);
        afterRecords.push_back(afterRec);
    }

    if (!beforeRecords.empty())
    {
        auto cmd = std::make_unique<TransformCommand>(&scene, beforeRecords, afterRecords, "Align Transform with View");
        CommandHistory::Get().Execute(std::move(cmd));
        scene.SetDirty(true);
    }
}
