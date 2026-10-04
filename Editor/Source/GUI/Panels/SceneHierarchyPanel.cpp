#include "SceneHierarchyPanel.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include <ImGui/tinyfiledialogs.h>
#include "AssetImporters/PrefabImporter.h"
#include "Core/Engine.h"
#include "Scene/Entity.h"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "Asset/AssetManager.h"
#include "Utils/ProjectManager.h"
#include "AssetImporters/ModelImporter.h"
#include "Utils/FileUtils.h"
#include "EditorSelection.h"
#include "EditorUtils.h"

using namespace rv;

static Ref<MeshAsset> ResolvePrimitiveMesh(const std::string& primitiveMeshName, AssetUUID& outMeshUUID)
{
    auto& manager = AssetManager::Get();
    auto& registry = manager.GetRegistry();
    auto projectAssetsPath = ProjectManager::GetAssetDirectory();
    auto primitivesDir = projectAssetsPath / "Models" / "Primitives";

    static const std::vector<std::string> extensions = { ".glb", ".fbx", ".obj" };

    std::filesystem::path foundModelPath;

    // Check primitive paths
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

        // try import primitives
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

    // fallback
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

static Entity LoadPrimitive(Scene& scene, const std::string& primitiveMeshName)
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

void SceneHierarchyPanel::Draw(Engine* engine)
{
    Scene& scene = engine->GetActiveScene();
    entt::entity rootEntity = scene.GetRootEntity();
    entt::registry& registry = scene.GetRegistry();

    std::vector<entt::entity> visibleOrder;

    auto isDescendantOfAny = [&](entt::entity e, const std::vector<entt::entity>& entityList) -> bool {
        entt::entity cur = e;
        while (cur != entt::null && registry.valid(cur) && scene.HasComponent<SceneTreeComponent>(cur)) {
            entt::entity parent = scene.GetComponent<SceneTreeComponent>(cur).parent;
            if (parent == entt::null || parent == rootEntity) break;
            if (std::find(entityList.begin(), entityList.end(), parent) != entityList.end()) {
                return true;
            }
            cur = parent;
        }
        return false;
    };

    auto selectSingle = [&](entt::entity e) {
        EditorSelection::Get().Select(e);
    };

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

    static char searchFilter[128] = "";
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.17f, 0.17f, 0.23f, 1.0f));
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##HierarchySearch", "Search Entities...", searchFilter, sizeof(searchFilter));
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto getChildren = [&scene, &registry](entt::entity parent) -> const std::vector<entt::entity>&{
        static const std::vector<entt::entity> empty;
        if (parent != entt::null && registry.valid(parent) && scene.HasComponent<SceneTreeComponent>(parent)) {
            return scene.GetComponent<SceneTreeComponent>(parent).children;
        }
        return empty;
        };

    auto matchesFilter = [&](entt::entity entity, auto& self) -> bool {
        if (searchFilter[0] == '\0') return true;
        if (!scene.HasComponent<TagComponent>(entity)) return false;
        auto& tagComp = scene.GetComponent<TagComponent>(entity);
        std::string nameLower = tagComp.tag;
        std::string filterLower = searchFilter;
        auto toLower = [](std::string& s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            };
        toLower(nameLower);
        toLower(filterLower);
        if (nameLower.find(filterLower) != std::string::npos) return true;

        for (auto child : getChildren(entity)) {
            if (self(child, self)) return true;
        }
        return false;
        };

    auto isDescendantOf = [&](entt::entity childCandidate, entt::entity parentCandidate, auto& self) -> bool {
        if (childCandidate == parentCandidate) return true;
        for (auto subChild : getChildren(childCandidate)) {
            if (self(subChild, parentCandidate, self)) return true;
        }
        return false;
        };

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.20f, 0.27f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.62f, 0.56f, 0.80f, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));

    std::function<void(entt::entity)> DrawEntityNode = [&](entt::entity entity)
        {
            if (!registry.valid(entity)) return;

            if (searchFilter[0] != '\0' && !matchesFilter(entity, matchesFilter))
                return;

            bool isRoot = (entity == rootEntity);
            const auto& children = getChildren(entity);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

            if (isRoot)
            {
                flags |= ImGuiTreeNodeFlags_DefaultOpen;
                flags |= ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }
            else if (children.empty())
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }

            if (!isRoot)
                visibleOrder.push_back(entity);

            bool isSelected = (!isRoot && EditorSelection::Get().IsSelected(entity));
            if (isSelected)
                flags |= ImGuiTreeNodeFlags_Selected;

            if (!scene.HasComponent<TagComponent>(entity))
                scene.AddComponent<TagComponent>(entity, "Entity");

            auto& tagComponent = scene.GetComponent<TagComponent>(entity);
            std::string nodeId = tagComponent.tag + "##" + std::to_string((uint32_t)entity);

            bool isPrefab = scene.HasComponent<PrefabComponent>(entity);
            bool isActiveInHierarchy = scene.IsEntityActive(entity);

            int colorPushes = 0;
            bool isPrimary = (isSelected && EditorSelection::Get().IsPrimary(entity) && EditorSelection::Get().GetCount() > 1);

            if (isPrimary)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.28f, 0.48f, 0.82f, 0.95f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.45f, 1.0f));
                colorPushes += 2;
            }
            else if (!isActiveInHierarchy)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.50f, 0.60f));
                colorPushes++;
            }
            else if (isPrefab)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.56f, 0.80f, 1.0f));
                colorPushes++;
            }

            std::string nodeTitle = tagComponent.tag;

            bool nodeOpen = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", nodeTitle.c_str());

            if (colorPushes > 0)
                ImGui::PopStyleColor(colorPushes);

            if (!isRoot && ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                ImGuiIO& io = ImGui::GetIO();
                if (io.KeyCtrl)
                {
                    EditorSelection::Get().Toggle(entity);
                }
                else if (io.KeyShift)
                {
                    entt::entity primary = EditorSelection::Get().GetPrimary();
                    if (primary != entt::null && !EditorSelection::Get().IsEmpty())
                    {
                        auto itA = std::find(visibleOrder.begin(), visibleOrder.end(), primary);
                        auto itB = std::find(visibleOrder.begin(), visibleOrder.end(), entity);
                        if (itA != visibleOrder.end() && itB != visibleOrder.end())
                        {
                            size_t idxA = std::distance(visibleOrder.begin(), itA);
                            size_t idxB = std::distance(visibleOrder.begin(), itB);
                            size_t start = std::min(idxA, idxB);
                            size_t end = std::max(idxA, idxB);
                            std::vector<entt::entity> range;
                            for (size_t i = start; i <= end; ++i)
                                range.push_back(visibleOrder[i]);
                            EditorSelection::Get().SelectRange(range, entity);
                        }
                        else
                        {
                            EditorSelection::Get().Select(entity);
                        }
                    }
                    else
                    {
                        EditorSelection::Get().Select(entity);
                    }
                }
                else
                {
                    EditorSelection::Get().Select(entity);
                }
            }
            else if (!isRoot && ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                if (!EditorSelection::Get().IsSelected(entity))
                {
                    EditorSelection::Get().Select(entity);
                }
            }

            if (!isRoot && ImGui::BeginPopupContextItem())
            {
                if (!EditorSelection::Get().IsSelected(entity))
                {
                    EditorSelection::Get().Select(entity);
                }

                if (ImGui::MenuItem("Create Child Entity"))
                {
                    entt::entity child = scene.CreateEntity("New Entity");
                    scene.SetParent(child, entity);
                    selectSingle(child);
                }

                if (ImGui::MenuItem("Save as prefab"))
                {
                    std::string entityName = scene.HasComponent<TagComponent>(entity)
                        ? scene.GetComponent<TagComponent>(entity).tag
                        : "prefab";
                    if (entityName.empty()) entityName = "prefab";
                    std::string defaultFileName = entityName + ".rprefab";

                    std::filesystem::path prefabsFolder = ProjectManager::GetAssetDirectory() / "Prefabs";
                    std::error_code ec;
                    if (!std::filesystem::exists(prefabsFolder, ec))
                        std::filesystem::create_directories(prefabsFolder, ec);

                    std::string defaultPath = (prefabsFolder / defaultFileName).string();

                    const char* filterPatterns[] = { "*.rprefab" };
                    const char* filePath = tinyfd_saveFileDialog("Create prefab as", defaultPath.c_str(), 1, filterPatterns, NULL);
                    if (filePath)
                    {
                        std::filesystem::path prefabPath = filePath;
                        if (prefabPath.extension() != ".rprefab")
                            prefabPath += ".rprefab";

                        AssetRegistry& reg = AssetManager::Get().GetRegistry();
                        AssetMeta meta = reg.GetOrCreateMeta(prefabPath);

                        Ref<PrefabAsset> prefab = PrefabImporter::CreatePrefabAsset(prefabPath, meta.uuid, scene, entity);
                        if (prefab)
                        {
                            meta.importerID = "PrefabImporter";
                            reg.SaveMeta(prefabPath, meta);
                            
                            auto assetDir = reg.GetAssetDir();
                            if (!assetDir.empty() && std::filesystem::exists(assetDir))
                                reg.Scan(assetDir);

                            if (!scene.HasComponent<PrefabComponent>(entity))
                                scene.AddComponent<PrefabComponent>(entity, meta.uuid);
                            else
                                scene.GetComponent<PrefabComponent>(entity).SetPrefabID(meta.uuid);

                            LOG_INFO("Prefab saved: {}", prefabPath.string());
                        }
                    }
                }

                if (scene.HasComponent<PrefabComponent>(entity))
                {
                    if (ImGui::MenuItem("Apply to Prefab"))
                    {
                        PrefabImporter::ApplyPrefab(scene, entity);
                    }

                    if (ImGui::MenuItem("Revert to Prefab"))
                    {
                        PrefabImporter::RevertPrefab(scene, entity);
                    }

                    if (ImGui::MenuItem("Unpack Prefab (Make Local)"))
                    {
                        scene.RemoveComponent<PrefabComponent>(entity);
                    }

                    ImGui::Separator();
                }

                if (ImGui::MenuItem("Detach from parent"))
                {
                    for (auto e : EditorSelection::Get().GetSelectedEntities())
                    {
                        if (e != entt::null && registry.valid(e))
                            scene.RemoveParent(e);
                    }
                }

                if (EditorSelection::Get().GetCount() > 1)
                {
                    if (ImGui::MenuItem("Duplicate Selected Entities", "Ctrl+D"))
                    {
                        const auto& selected = EditorSelection::Get().GetSelectedEntities();
                        entt::entity primary = EditorSelection::Get().GetPrimary();
                        std::vector<entt::entity> newSelection;
                        entt::entity newPrimary = entt::null;
                        for (auto e : selected)
                        {
                            if (e != entt::null && registry.valid(e) && !isDescendantOfAny(e, selected))
                            {
                                Entity duplicated = scene.DuplicateEntity(e);
                                if (duplicated.GetHandle() != entt::null)
                                {
                                    newSelection.push_back(duplicated.GetHandle());
                                    if (e == primary)
                                        newPrimary = duplicated.GetHandle();
                                }
                            }
                        }
                        if (!newSelection.empty())
                        {
                            EditorSelection::Get().SetSelection(newSelection, newPrimary);
                        }
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Duplicate Entity", "Ctrl+D"))
                    {
                        Entity duplicated = scene.DuplicateEntity(entity);
                        if (duplicated.GetHandle() != entt::null)
                        {
                            selectSingle(duplicated.GetHandle());
                        }
                    }
                }

                ImGui::Separator();

                if (EditorSelection::Get().GetCount() > 1)
                {
                    if (ImGui::MenuItem("Delete Selected Entities", "Del"))
                    {
                        for (auto e : EditorSelection::Get().GetSelectedEntities())
                        {
                            if (e != entt::null && registry.valid(e))
                                scene.QueueDestroyEntity(e);
                        }
                        EditorSelection::Get().Clear();
                    }
                }
                else
                {
                    if (ImGui::MenuItem("Delete Entity", "Del"))
                    {
                        scene.QueueDestroyEntity(entity);
                        EditorSelection::Get().Clear();
                    }
                }
                ImGui::EndPopup();
            }

            if (!isRoot && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                entt::entity e = entity;
                ImGui::SetDragDropPayload("ENTITY_DRAG", &e, sizeof(entt::entity));
                ImGui::Text("%s", tagComponent.tag.c_str());
                ImGui::EndDragDropSource();
            }

            if (!isRoot && ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG", ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
                {
                    ImVec2 itemMin = ImGui::GetItemRectMin();
                    ImVec2 itemMax = ImGui::GetItemRectMax();
                    float itemHeight = itemMax.y - itemMin.y;
                    float mousePosY = ImGui::GetMousePos().y;

                    float topBorder = itemMin.y + (itemHeight * 0.25f);
                    float bottomBorder = itemMax.y - (itemHeight * 0.25f);

                    ImDrawList* drawList = ImGui::GetWindowDrawList();

                    if (mousePosY < topBorder)
                    {
                        drawList->AddLine(ImVec2(itemMin.x, itemMin.y), ImVec2(itemMax.x, itemMin.y), IM_COL32(255, 204, 0, 255), 2.0f);
                        if (payload->IsDelivery())
                        {
                            entt::entity dragged = *(entt::entity*)payload->Data;
                            if (dragged != entity && !isDescendantOf(dragged, entity, isDescendantOf))
                            {
                                scene.MoveChildOrder(dragged, entity, true);
                            }
                        }
                    }
                    else if (mousePosY > bottomBorder)
                    {
                        drawList->AddLine(ImVec2(itemMin.x, itemMax.y), ImVec2(itemMax.x, itemMax.y), IM_COL32(255, 204, 0, 255), 2.0f);
                        if (payload->IsDelivery())
                        {
                            entt::entity dragged = *(entt::entity*)payload->Data;
                            if (dragged != entity && !isDescendantOf(dragged, entity, isDescendantOf))
                            {
                                scene.MoveChildOrder(dragged, entity, false);
                            }
                        }
                    }
                    else
                    {
                        drawList->AddRect(itemMin, itemMax, IM_COL32(0, 180, 255, 255), 0.0f, 0, 1.5f);
                        if (payload->IsDelivery())
                        {
                            entt::entity dragged = *(entt::entity*)payload->Data;
                            if (dragged != entity && !isDescendantOf(dragged, entity, isDescendantOf))
                            {
                                scene.SetParent(dragged, entity);
                            }
                        }
                    }
                }

                if (const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
                {
                    std::string pathStr((const char*)assetPayload->Data);
                    if (pathStr.ends_with(".rprefab"))
                    {
                        AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                        if (uuid.IsValid())
                        {
                            Entity inst = scene.Instantiate(uuid, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), entity);
                            if (inst.GetHandle() != entt::null)
                                EditorSelection::Get().Select(inst.GetHandle());
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (isRoot)
            {
                auto childrenCopy = children;
                for (auto child : childrenCopy)
                    DrawEntityNode(child);
            }
            else if (nodeOpen)
            {
                auto childrenCopy = children;
                for (auto child : childrenCopy)
                    DrawEntityNode(child);

                ImGui::TreePop();
            }
        };

    auto rootChildren = getChildren(rootEntity);
    for (auto entity : rootChildren)
    {
        DrawEntityNode(entity);
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->Rect(), ImGui::GetID("SceneHierarchyEmptyDrop")))
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG"))
        {
            entt::entity dragged = *(entt::entity*)payload->Data;
            bool draggedInSelection = EditorSelection::Get().IsSelected(dragged);
            std::vector<entt::entity> toMove = (draggedInSelection && EditorSelection::Get().GetCount() > 1) ? EditorSelection::Get().GetSelectedEntities() : std::vector<entt::entity>{ dragged };
            for (auto targetMove : toMove)
            {
                if (targetMove != entt::null && registry.valid(targetMove) && !isDescendantOfAny(targetMove, toMove))
                {
                    scene.SetParent(targetMove, rootEntity);
                }
            }
        }

        if (const ImGuiPayload* assetPayload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
        {
            std::string pathStr((const char*)assetPayload->Data);
            if (pathStr.ends_with(".rprefab"))
            {
                AssetUUID uuid = EditorUtils::ReadUUIDFromMeta(pathStr);
                if (uuid.IsValid())
                {
                    Entity inst = scene.Instantiate(uuid, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), rootEntity);
                    if (inst.GetHandle() != entt::null)
                        EditorSelection::Get().Select(inst.GetHandle());
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
    {
        EditorSelection::Get().Clear();
    }

    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Entity"))
        {
            selectSingle(scene.CreateEntity("New Entity"));
        }

        if (ImGui::MenuItem("Camera"))
        {
            entt::entity cam = scene.CreateEntity("Camera");
            scene.AddComponent<CameraComponent>(cam);
            selectSingle(cam);
        }

        if (ImGui::MenuItem("Particle Emitter"))
        {
            Entity emitterEntity = LoadPrimitive(scene, "Quad");
            if (emitterEntity)
            {
                if (emitterEntity.HasComponent<RigidbodyComponent>())
                    emitterEntity.RemoveComponent<RigidbodyComponent>();
                if (emitterEntity.HasComponent<ConvexHullColliderComponent>())
                    emitterEntity.RemoveComponent<ConvexHullColliderComponent>();

                emitterEntity.GetComponent<TagComponent>().tag = "Particle Emitter";
                emitterEntity.AddComponent<ParticleEmitterComponent>();
                selectSingle(emitterEntity ? emitterEntity.GetHandle() : entt::null);
            }
        }


        if (ImGui::MenuItem("Audio Emitter"))
        {
            entt::entity audio = scene.CreateEntity("AudioEmitter");
            scene.AddComponent<AudioEmitterComponent>(audio);
            selectSingle(audio);
        }

        if (ImGui::BeginMenu("Primitives"))
        {
            if (ImGui::MenuItem("Cube")) selectSingle(LoadPrimitive(scene, "Cube"));
            if (ImGui::MenuItem("Sphere")) selectSingle(LoadPrimitive(scene, "Sphere"));
            if (ImGui::MenuItem("Cylinder")) selectSingle(LoadPrimitive(scene, "Cylinder"));
            if (ImGui::MenuItem("Quad")) selectSingle(LoadPrimitive(scene, "Quad"));
            if (ImGui::MenuItem("Cone")) selectSingle(LoadPrimitive(scene, "Cone"));
            if (ImGui::MenuItem("Capsule")) selectSingle(LoadPrimitive(scene, "Capsule"));
            if (ImGui::MenuItem("Torus")) selectSingle(LoadPrimitive(scene, "Torus"));
            if (ImGui::MenuItem("Plane")) selectSingle(LoadPrimitive(scene, "Plane"));
            if (ImGui::MenuItem("Monkey")) selectSingle(LoadPrimitive(scene, "Monkey"));
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights"))
        {
            if (ImGui::MenuItem("Directional Light"))
            {
                entt::entity light = scene.CreateEntity("DirectionalLight");
                scene.GetComponent<TransformComponent>(light).SetEulerRotation(glm::vec3(-60.0f, -90.0f, 0.0f));
                scene.AddComponent<DirectionalLightComponent>(light);
                selectSingle(light);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                entt::entity light = scene.CreateEntity("PointLight");
                scene.AddComponent<PointLightComponent>(light);
                selectSingle(light);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("UI"))
        {
            if (ImGui::MenuItem("UI Canvas"))
            {
                entt::entity canvas = scene.CreateEntity("Canvas");
                scene.AddComponent<UICanvasComponent>(canvas);
                scene.AddComponent<RectTransformComponent>(canvas);
                selectSingle(canvas);
            }
            if (ImGui::MenuItem("UI Text"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity text = scene.CreateEntity("UI Text");
                scene.AddComponent<RectTransformComponent>(text);
                scene.AddComponent<UITextComponent>(text);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(text, parent);
                }
                selectSingle(text);
            }
            if (ImGui::MenuItem("UI Button"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity button = scene.CreateEntity("UI Button");
                scene.AddComponent<RectTransformComponent>(button);
                scene.AddComponent<UIButtonComponent>(button);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(button, parent);
                }
                selectSingle(button);
            }
            if (ImGui::MenuItem("UI Image"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity img = scene.CreateEntity("UI Image");
                scene.AddComponent<RectTransformComponent>(img);
                scene.AddComponent<UIImageComponent>(img);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(img, parent);
                }
                selectSingle(img);
            }
            if (ImGui::MenuItem("UI Slider"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity slider = scene.CreateEntity("UI Slider");
                scene.AddComponent<RectTransformComponent>(slider);
                scene.AddComponent<UISliderComponent>(slider);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(slider, parent);
                }
                selectSingle(slider);
            }
            if (ImGui::MenuItem("UI Progress Bar"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity pbar = scene.CreateEntity("UI Progress Bar");
                scene.AddComponent<RectTransformComponent>(pbar);
                scene.AddComponent<UIProgressBarComponent>(pbar);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(pbar, parent);
                }
                selectSingle(pbar);
            }
            if (ImGui::MenuItem("UI Checkbox"))
            {
                entt::entity parent = EditorSelection::Get().GetPrimary();
                entt::entity cb = scene.CreateEntity("UI Checkbox");
                scene.AddComponent<RectTransformComponent>(cb);
                scene.AddComponent<UICheckboxComponent>(cb);
                if (registry.valid(parent) && parent != rootEntity)
                {
                    scene.SetParent(cb, parent);
                }
                selectSingle(cb);
            }
            ImGui::EndMenu();
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}