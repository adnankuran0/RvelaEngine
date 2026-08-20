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

using namespace rv;

struct PrimitiveConfig { std::string name; AssetUUID uuid; };
static Entity LoadPrimitive(Scene& scene, const std::string& primitiveMeshName)
{
    static const std::unordered_map<std::string, PrimitiveConfig> map =
    {
        {"Cube", {"Cube", AssetUUID::FromString("55dee74b-34c1-4aac-80c9-627b95a8cf58")}},
        {"Sphere", {"Sphere", AssetUUID::FromString("b6c7f2a2-cce2-4b68-be50-19a8f552e727")}},
        {"Cylinder", {"Cylinder", AssetUUID::FromString("2c4d9b01-fea1-4eec-b741-bb054229ba61")}},
        {"Quad", {"Quad", AssetUUID::FromString("1d2596bb-18d3-41a6-9848-a8a584870ce3")}},
        {"Cone", {"Cone", AssetUUID::FromString("92ffc5c9-9f6f-4332-ad83-e1d03b046a53")}},
        {"Capsule", {"Capsule", AssetUUID::FromString("62345031-f93b-40f4-aa76-18c59801997d")}},
        {"Plane", {"Plane", AssetUUID::FromString("077f6760-e8d5-44b4-895c-5d88be2db952")}},
        {"Monkey", {"Monkey", AssetUUID::FromString("d5b8e68a-8170-4b1a-b439-62ae98d391f8")}},
        {"Torus", {"Torus", AssetUUID::FromString("db65917a-cf92-4335-addb-f30e2608d318")}}
    };
    auto it = map.find(primitiveMeshName);
    if (it == map.end()) return Entity{};
    const auto& cfg = it->second;
    Entity root = scene.CreateEntity(cfg.name);

    Ref<MeshAsset> m = AssetManager::Get().GetAsset<MeshAsset>(cfg.uuid);
    if (!m)
    {
        LOG_ERROR("Mesh not found: {}", cfg.name);
        return root;
    }
    root.AddComponent<MeshComponent>(m->GetUUID());
    root.AddComponent<MeshRendererComponent>(m);
    root.GetComponent<TagComponent>().tag = cfg.name;
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

void SceneHierarchyPanel::Draw(Engine* engine, entt::entity& selectedEntity)
{
    Scene& scene = engine->GetActiveScene();
    entt::entity rootEntity = scene.GetRootEntity();
    entt::registry& registry = scene.GetRegistry();

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

    auto getChildren = [&registry](entt::entity parent) {
        std::vector<entt::entity> children;
        registry.view<SceneTreeComponent>().each([&](entt::entity e, SceneTreeComponent& stc) {
            if (stc.parent == parent) {
                children.push_back(e);
            }
            });
        return children;
        };

    auto matchesFilter = [&](entt::entity entity, auto& self) -> bool {
        if (searchFilter[0] == '\0') return true;
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

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.20f, 0.20f, 0.27f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.55f, 0.50f, 0.72f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.62f, 0.56f, 0.80f, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 2.0f));

    std::function<void(entt::entity)> DrawEntityNode = [&](entt::entity entity)
        {
            if (searchFilter[0] != '\0' && !matchesFilter(entity, matchesFilter))
                return;

            bool isRoot = (entity == rootEntity);
            auto children = getChildren(entity);

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

            if (!isRoot && entity == selectedEntity)
                flags |= ImGuiTreeNodeFlags_Selected;

            auto& tagComponent = scene.GetComponent<TagComponent>(entity);
            std::string nodeId = tagComponent.tag + "##" + std::to_string((uint32_t)entity);

            bool isPrefab = scene.HasComponent<PrefabComponent>(entity);
            if (isPrefab)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.56f, 0.80f, 1.0f));

            bool nodeOpen = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tagComponent.tag.c_str());

            if (isPrefab)
                ImGui::PopStyleColor();

            if (!isRoot && (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)))
                selectedEntity = entity;

            if (!isRoot && ImGui::BeginPopupContextItem())
            {
                selectedEntity = entity;

                if (ImGui::MenuItem("Create Child Entity"))
                {
                    entt::entity child = scene.CreateEntity("New Entity");
                    scene.SetParent(child, entity);
                    selectedEntity = child;
                }

                if (ImGui::MenuItem("Delete Entity"))
                {
                    scene.QueueDestroyEntity(entity);
                    if (selectedEntity == entity)
                        selectedEntity = entt::null;
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
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG"))
                {
                    entt::entity child = *(entt::entity*)payload->Data;
                    if (child != entity)
                        scene.SetParent(child, entity);
                }
                ImGui::EndDragDropTarget();
            }

            if (isRoot)
            {
                for (auto child : children)
                    DrawEntityNode(child);
            }
            else if (nodeOpen)
            {
                for (auto child : children)
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

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        selectedEntity = entt::null;

    if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Entity")) {
            selectedEntity = scene.CreateEntity("New Entity");
        }

        if (ImGui::MenuItem("Camera")) {
            selectedEntity = scene.CreateEntity("Camera");
            scene.AddComponent<CameraComponent>(selectedEntity);
        }

        if (ImGui::MenuItem("Particle Emitter")) {
            Entity emitterEntity = LoadPrimitive(scene, "Quad");
            if (emitterEntity)
            {
                if(emitterEntity.HasComponent<RigidbodyComponent>())
                    emitterEntity.RemoveComponent<RigidbodyComponent>();
                if (emitterEntity.HasComponent<ConvexHullColliderComponent>())
                    emitterEntity.RemoveComponent<ConvexHullColliderComponent>();

                emitterEntity.GetComponent<TagComponent>().tag = "Particle Emitter";
                emitterEntity.AddComponent<ParticleEmitterComponent>();
                selectedEntity = emitterEntity ? emitterEntity.GetHandle() : entt::null;
            }
        }

        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Cube"))
                selectedEntity = LoadPrimitive(scene, "Cube");
            if (ImGui::MenuItem("Sphere"))
                selectedEntity = LoadPrimitive(scene, "Sphere");
            if (ImGui::MenuItem("Cylinder"))
                selectedEntity = LoadPrimitive(scene, "Cylinder");
            if (ImGui::MenuItem("Quad"))
                selectedEntity = LoadPrimitive(scene, "Quad");
            if (ImGui::MenuItem("Cone"))
                selectedEntity = LoadPrimitive(scene, "Cone");
            if (ImGui::MenuItem("Capsule"))
                selectedEntity = LoadPrimitive(scene, "Capsule");
            if (ImGui::MenuItem("Torus"))
                selectedEntity = LoadPrimitive(scene, "Torus");
            if (ImGui::MenuItem("Plane"))
                selectedEntity = LoadPrimitive(scene, "Plane");
            if (ImGui::MenuItem("Monkey"))
                selectedEntity = LoadPrimitive(scene, "Monkey");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights")) {
            if (ImGui::MenuItem("Directional Light"))
            {
                selectedEntity = scene.CreateEntity("DirectionalLight");
                scene.GetComponent<TransformComponent>(selectedEntity).SetEulerRotation(glm::vec3(-60.0f, -90.0f, 0.0f));
                scene.AddComponent<DirectionalLightComponent>(selectedEntity);
            }
            if (ImGui::MenuItem("Point Light"))
            {
                selectedEntity = scene.CreateEntity("PointLight");
                scene.AddComponent<PointLightComponent>(selectedEntity);
            }
            if (ImGui::MenuItem("Spot Light"))
            {

            }
            if (ImGui::MenuItem("Area Light"))
            {

            }

            ImGui::EndMenu();
        }

        if (selectedEntity != entt::null && registry.valid(selectedEntity))
        {
            ImGui::Separator();

            if (ImGui::MenuItem("Save as prefab"))
            {
                const char* filterPatterns[] = { "*.rprefab" };
                const char* filePath = tinyfd_saveFileDialog(
                    "Create prefab as", "prefab.rprefab", 1, filterPatterns, NULL);

                if (filePath)
                {
                    std::filesystem::path prefabPath = filePath;

                    AssetRegistry& reg = AssetManager::Get().GetRegistry();
                    AssetMeta meta = reg.GetOrCreateMeta(prefabPath);

                    Ref<PrefabAsset> prefab = PrefabImporter::CreatePrefabAsset(
                        prefabPath, meta.uuid, scene, selectedEntity);

                    if (prefab)
                    {
                        meta.importerID = "PrefabImporter";
                        reg.SaveMeta(prefabPath, meta);
                        reg.Scan(reg.GetAssetDir());
                        LOG_INFO("Prefab saved: {}", prefabPath.string());
                    }
                }
            }

            if (scene.HasComponent<PrefabComponent>(selectedEntity))
            {
                if (ImGui::MenuItem("Make local"))
                {
                    scene.RemoveComponent<PrefabComponent>(selectedEntity);
                }
            }

            if (ImGui::MenuItem("Detach from parent")) {
                scene.RemoveParent(selectedEntity);
            }

            if (ImGui::MenuItem("Delete Entity")) {
                scene.DestroyEntity(selectedEntity);
                selectedEntity = entt::null;
            }
        }

        ImGui::EndPopup();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}