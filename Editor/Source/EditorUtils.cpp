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
#include "Scene/Components/MeshRendererComponent.h"
#include "Scene/Components/ColliderComponents.h"
#include "Physics/PhysicsSystem.h"
#include "Physics/PhysicsWorld.h"
#include "AssetImporters/ModelImporter.h"
#include "Utils/FileUtils.h"
#include "Scene/Components/MeshComponent.h"
#include "Scene/Components/MaterialComponent.h"
#include "Scene/Components/RigidbodyComponent.h"
#include "Scene/Components/TagComponent.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/constants.hpp>
#include <functional>
#include <algorithm>

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

static glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest)
{
    start = glm::normalize(start);
    dest = glm::normalize(dest);

    float cosTheta = glm::dot(start, dest);
    glm::vec3 rotationAxis;

    if (cosTheta < -1.0f + 0.001f)
    {
        rotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
        if (glm::length2(rotationAxis) < 0.01f)
            rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);
        rotationAxis = glm::normalize(rotationAxis);
        return glm::angleAxis(glm::pi<float>(), rotationAxis);
    }

    if (cosTheta > 1.0f - 0.0001f)
    {
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    rotationAxis = glm::cross(start, dest);
    float s = std::sqrt((1.0f + cosTheta) * 2.0f);
    float invs = 1.0f / s;

    return glm::normalize(glm::quat(
        s * 0.5f,
        rotationAxis.x * invs,
        rotationAxis.y * invs,
        rotationAxis.z * invs
    ));
}

void EditorUtils::SnapSelectedToFloor(Engine& engine, bool alignWithNormal)
{
    Scene& scene = engine.GetActiveScene();
    if (scene.GetState() != SceneState::EDIT)
        return;

    auto& reg = scene.GetRegistry();
    const auto& selectedEntities = EditorSelection::Get().GetSelectedEntities();
    if (selectedEntities.empty())
        return;

    scene.GetTransformSystem().Update();
    scene.GetPhysicsSystem().Update();

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

    std::vector<entt::entity> ignoredEntities;
    std::function<void(entt::entity)> collectDescendants = [&](entt::entity parent) {
        if (!scene.HasComponent<SceneTreeComponent>(parent)) return;
        const auto& children = scene.GetComponent<SceneTreeComponent>(parent).children;
        for (auto child : children)
        {
            if (child != entt::null && reg.valid(child))
            {
                ignoredEntities.push_back(child);
                collectDescendants(child);
            }
        }
    };

    for (auto sel : selectedEntities)
    {
        if (sel != entt::null && reg.valid(sel))
        {
            ignoredEntities.push_back(sel);
            collectDescendants(sel);
        }
    }

    auto computeBottomOffset = [&](entt::entity rootEntity, const glm::quat& newRot, const glm::vec3& scale) -> float {
        float minOffsetY = 0.0f;
        bool found = false;

        glm::mat4 candidateRoot = glm::mat4_cast(newRot) * glm::scale(glm::mat4(1.0f), scale);

        auto checkEntityMesh = [&](entt::entity e, const glm::mat4& relTransform) {
            if (reg.any_of<MeshRendererComponent>(e))
            {
                auto& mrc = reg.get<MeshRendererComponent>(e);
                if (mrc.localAABB.IsValid())
                {
                    glm::mat4 fullMatrix = candidateRoot * relTransform;
                    AABB transformed = mrc.localAABB.CalculateWorldAABB(fullMatrix);
                    if (!found)
                    {
                        minOffsetY = transformed.min.y;
                        found = true;
                    }
                    else
                    {
                        minOffsetY = glm::min(minOffsetY, transformed.min.y);
                    }
                }
            }
        };

        checkEntityMesh(rootEntity, glm::mat4(1.0f));

        if (scene.HasComponent<SceneTreeComponent>(rootEntity))
        {
            glm::mat4 invRootWorld = glm::inverse(reg.get<TransformComponent>(rootEntity).GetWorldMatrix());
            std::function<void(entt::entity)> visitChildren = [&](entt::entity cur) {
                if (!scene.HasComponent<SceneTreeComponent>(cur)) return;
                const auto& children = scene.GetComponent<SceneTreeComponent>(cur).children;
                for (auto child : children)
                {
                    if (child != entt::null && reg.valid(child) && reg.any_of<TransformComponent>(child))
                    {
                        glm::mat4 childWorld = reg.get<TransformComponent>(child).GetWorldMatrix();
                        glm::mat4 rel = invRootWorld * childWorld;
                        checkEntityMesh(child, rel);
                        visitChildren(child);
                    }
                }
            };
            visitChildren(rootEntity);
        }

        if (!found)
        {
            if (reg.any_of<BoxColliderComponent>(rootEntity))
            {
                auto& box = reg.get<BoxColliderComponent>(rootEntity);
                glm::vec3 halfSize = box.GetSize() * 0.5f * scale;
                glm::vec3 offset = box.GetOffset() * scale;
                AABB boxAABB(offset - halfSize, offset + halfSize);
                AABB transformed = boxAABB.CalculateWorldAABB(candidateRoot);
                minOffsetY = transformed.min.y;
                found = true;
            }
            else if (reg.any_of<SphereColliderComponent>(rootEntity))
            {
                auto& sphere = reg.get<SphereColliderComponent>(rootEntity);
                float maxS = glm::max(scale.x, glm::max(scale.y, scale.z));
                float r = sphere.GetRadius() * maxS;
                minOffsetY = (sphere.GetOffset().y * scale.y) - r;
                found = true;
            }
            else if (reg.any_of<CapsuleColliderComponent>(rootEntity))
            {
                auto& cap = reg.get<CapsuleColliderComponent>(rootEntity);
                float totalH = (cap.GetHalfHeight() + cap.GetRadius()) * scale.y;
                minOffsetY = (cap.GetOffset().y * scale.y) - totalH;
                found = true;
            }
        }

        return found ? minOffsetY : 0.0f;
    };

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

        glm::mat4 curWorldMatrix = tc.GetWorldMatrix();
        glm::vec3 curWorldPos = tc.GetWorldPosition();
        glm::quat curWorldRot = tc.GetWorldRotation();
        glm::vec3 curWorldScale = tc.GetWorldScale();

        float initialBottomOffset = computeBottomOffset(e, curWorldRot, curWorldScale);

        float rayNudgeY = glm::max(0.1f, -initialBottomOffset + 0.1f);
        glm::vec3 rayStart = curWorldPos + glm::vec3(0.0f, rayNudgeY, 0.0f);
        float rayDistance = 5000.0f;

        Physics::RaycastResult hitResult = scene.GetPhysicsSystem().GetPhysicsWorld().Raycast(
            rayStart,
            glm::vec3(0.0f, -1.0f, 0.0f),
            rayDistance,
            false,
            ignoredEntities
        );

        bool hitFound = false;
        glm::vec3 hitPoint(0.0f);
        glm::vec3 hitNormal(0.0f, 1.0f, 0.0f);

        if (hitResult.hit)
        {
            hitFound = true;
            hitPoint = hitResult.point;
            hitNormal = hitResult.normal;
        }
        else
        {
            float bestY = -FLT_MAX;
            auto meshView = reg.view<MeshRendererComponent, TransformComponent>();
            for (auto other : meshView)
            {
                if (std::find(ignoredEntities.begin(), ignoredEntities.end(), other) != ignoredEntities.end())
                    continue;

                const auto& otherMesh = meshView.get<MeshRendererComponent>(other);
                if (!otherMesh.worldAABB.IsValid()) continue;

                if (rayStart.x >= otherMesh.worldAABB.min.x && rayStart.x <= otherMesh.worldAABB.max.x &&
                    rayStart.z >= otherMesh.worldAABB.min.z && rayStart.z <= otherMesh.worldAABB.max.z)
                {
                    if (otherMesh.worldAABB.max.y <= rayStart.y + 0.1f && otherMesh.worldAABB.max.y > bestY)
                    {
                        bestY = otherMesh.worldAABB.max.y;
                        hitFound = true;
                        hitPoint = glm::vec3(rayStart.x, bestY, rayStart.z);
                        hitNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                    }
                }
            }

            if (!hitFound)
            {
                hitFound = true;
                hitPoint = glm::vec3(rayStart.x, 0.0f, rayStart.z);
                hitNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }

        if (glm::length(hitNormal) > 0.001f)
            hitNormal = glm::normalize(hitNormal);
        else
            hitNormal = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::quat newWorldRot = curWorldRot;
        if (alignWithNormal && hitNormal.y > 0.1f)
        {
            glm::vec3 curUp = glm::normalize(glm::vec3(curWorldMatrix[1]));
            glm::quat deltaRot = RotationBetweenVectors(curUp, hitNormal);
            newWorldRot = glm::normalize(deltaRot * curWorldRot);
        }

        float finalBottomOffset = alignWithNormal ? computeBottomOffset(e, newWorldRot, curWorldScale) : initialBottomOffset;

        glm::vec3 newWorldPos = glm::vec3(curWorldPos.x, hitPoint.y - finalBottomOffset, curWorldPos.z);

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
            tc.SetPosition(newWorldPos);
            if (alignWithNormal)
                tc.SetRotation(newWorldRot);
        }
        else
        {
            glm::mat4 invParentWorld = glm::inverse(parentWorld);
            tc.SetPosition(glm::vec3(invParentWorld * glm::vec4(newWorldPos, 1.0f)));
            if (alignWithNormal)
                tc.SetRotation(glm::normalize(glm::inverse(parentWorldRot) * newWorldRot));
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
        auto cmd = std::make_unique<TransformCommand>(&scene, beforeRecords, afterRecords, "Snap to Floor");
        CommandHistory::Get().Execute(std::move(cmd));
        scene.SetDirty(true);
    }
}

static Ref<MeshAsset> ResolvePrimitiveMesh(const std::string& primitiveMeshName, AssetUUID& outMeshUUID)
{
    auto& manager = AssetManager::Get();
    auto& registry = manager.GetRegistry();
    auto projectAssetsPath = ProjectManager::GetAssetDirectory();
    auto primitivesDir = projectAssetsPath / "Models" / "Primitives";

    static const std::vector<std::string> extensions = { ".glb", ".fbx", ".obj" };

    std::filesystem::path foundModelPath;

    for (const auto& ext : extensions)
    {
        auto candidate = primitivesDir / (primitiveMeshName + ext);
        if (std::filesystem::exists(candidate))
        {
            foundModelPath = candidate;
            break;
        }
    }

    if (foundModelPath.empty() && std::filesystem::exists(projectAssetsPath))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectAssetsPath))
        {
            if (!entry.is_regular_file()) continue;
            const auto& p = entry.path();
            if (p.extension() == ".rmeta") continue;
            if (p.stem().string() == primitiveMeshName)
            {
                std::string ext = p.extension().string();
                if (ext == ".glb" || ext == ".fbx" || ext == ".obj")
                {
                    foundModelPath = p;
                    break;
                }
            }
        }
    }

    if (!foundModelPath.empty())
    {
        AssetUUID modelUUID = registry.GetUUID(foundModelPath);
        if (!modelUUID.IsValid())
        {
            AssetMeta meta = registry.GetOrCreateMeta(foundModelPath);
            modelUUID = meta.uuid;
        }

        if (modelUUID.IsValid())
        {
            AssetMeta meta = registry.GetMeta(modelUUID);
            for (const auto& sub : meta.subAssets)
            {
                if (sub.type == "Mesh" && sub.uuid.IsValid())
                {
                    Ref<MeshAsset> mesh = manager.GetAsset<MeshAsset>(sub.uuid);
                    if (mesh)
                    {
                        outMeshUUID = sub.uuid;
                        return mesh;
                    }
                }
            }
        }

        ModelImporter importer;
        ModelImportSettings settings;
        ModelImportResult result = importer.ImportModel(foundModelPath, registry, settings);
        for (const auto& [idx, uuid] : result.meshUUIDs)
        {
            Ref<MeshAsset> mesh = manager.GetAsset<MeshAsset>(uuid);
            if (mesh)
            {
                outMeshUUID = uuid;
                return mesh;
            }
        }
    }

    static const std::unordered_map<std::string, AssetUUID> legacyMap =
    {
        {"Cube", AssetUUID::FromString("55dee74b-34c1-4aac-80c9-627b95a8cf58")},
        {"Sphere", AssetUUID::FromString("b6c7f2a2-cce2-4b68-be50-19a8f552e727")},
        {"Cylinder", AssetUUID::FromString("2c4d9b01-fea1-4eec-b741-bb054229ba61")},
        {"Quad", AssetUUID::FromString("1d2596bb-18d3-41a6-9848-a8a584870ce3")},
        {"Cone", AssetUUID::FromString("92ffc5c9-9f6f-4332-ad83-e1d03b046a53")},
        {"Capsule", AssetUUID::FromString("62345031-f93b-40f4-aa76-18c59801997d")},
        {"Plane", AssetUUID::FromString("077f6760-e8d5-44b4-895c-5d88be2db952")},
        {"Monkey", AssetUUID::FromString("d5b8e68a-8170-4b1a-b439-62ae98d391f8")},
        {"Torus", AssetUUID::FromString("db65917a-cf92-4335-addb-f30e2608d318")}
    };

    auto it = legacyMap.find(primitiveMeshName);
    if (it != legacyMap.end())
    {
        Ref<MeshAsset> mesh = manager.GetAsset<MeshAsset>(it->second);
        if (mesh)
        {
            outMeshUUID = it->second;
            return mesh;
        }
    }

    auto templatePrimitive = EDITOR_PATH("TemplateProject/Assets/Models/Primitives/" + primitiveMeshName + ".glb").GetAbsolute();
    if (std::filesystem::exists(templatePrimitive))
    {
        std::filesystem::create_directories(primitivesDir);
        auto destModel = primitivesDir / (primitiveMeshName + ".glb");
        std::error_code ec;
        std::filesystem::copy_file(templatePrimitive, destModel, std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec)
        {
            ModelImporter importer;
            ModelImportSettings settings;
            ModelImportResult result = importer.ImportModel(destModel, registry, settings);
            for (const auto& [idx, uuid] : result.meshUUIDs)
            {
                Ref<MeshAsset> mesh = manager.GetAsset<MeshAsset>(uuid);
                if (mesh)
                {
                    outMeshUUID = uuid;
                    return mesh;
                }
            }
        }
    }

    return nullptr;
}

Entity EditorUtils::LoadPrimitive(Scene& scene, const std::string& primitiveMeshName)
{
    AssetUUID meshUUID = AssetUUID::Invalid();
    Ref<MeshAsset> m = ResolvePrimitiveMesh(primitiveMeshName, meshUUID);
    if (!m)
    {
        LOG_ERROR("Mesh not found for primitive: {}", primitiveMeshName);
        return Entity{};
    }

    Entity root = scene.CreateEntity(primitiveMeshName);
    root.AddComponent<MeshComponent>(m->GetUUID());
    root.AddComponent<MeshRendererComponent>(m);
    root.GetComponent<TagComponent>().tag = primitiveMeshName;
    root.AddComponent<MaterialComponent>();
    root.AddComponent<RigidbodyComponent>();
    if (primitiveMeshName == "Cube")
        root.AddComponent<BoxColliderComponent>();
    else if (primitiveMeshName == "Sphere")
        root.AddComponent<SphereColliderComponent>();
    else if (primitiveMeshName == "Capsule")
        root.AddComponent<CapsuleColliderComponent>();
    else if (primitiveMeshName == "Cylinder")
        root.AddComponent<CylinderColliderComponent>();
    else
        root.AddComponent<ConvexHullColliderComponent>();

    scene.SetParent(root.GetHandle(), scene.GetRootEntity());
    return root;
}
