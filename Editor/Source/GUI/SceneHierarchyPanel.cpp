#include "SceneHierarchyPanel.h"
#include "ImGui/imgui.h"
#include <ImGui/tinyfiledialogs.h>
#include "AssetImporters/PrefabImporter.h"
#include "Core/Engine.h"
#include "Scene/Entity.h"

using namespace rv;

struct PrimitiveConfig { std::string name; AssetUUID uuid; };
static Entity LoadPrimitive(Scene& scene, const std::string& primitiveMeshName)
{
    static const std::unordered_map<std::string, PrimitiveConfig> map =
    {
        {"Cube", {"Cube", AssetUUID::FromString("55dee74b-34c1-4aac-80c9-627b95a8cf58")}},
        {"Sphere", {"Sphere", AssetUUID::FromString("b6c7f2a2-cce2-4b68-be50-19a8f552e727")}},
        {"Cylinder", {"Cylinder", AssetUUID::FromString("2c4d9b01-fea1-4eec-b741-bb054229ba61")}},
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
    AssetUUID defaultMaterialId = AssetUUID::FromString("ee3dde12-6263-4f11-bb1d-812b3e196ab7");
    root.AddComponent<MaterialComponent>(defaultMaterialId);
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
    ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

    auto rootEntities = scene.GetRootEntities();

    auto getChildren = [&registry](entt::entity parent) {
        std::vector<entt::entity> children;
        registry.view<SceneTreeComponent>().each([&](entt::entity e, SceneTreeComponent& stc) {
            if (stc.parent == parent) {
                children.push_back(e);
            }
            });
        return children;
        };


    std::function<void(entt::entity)> DrawEntityNode = [&](entt::entity entity)
        {
            bool isRoot = (entity == rootEntity);

            auto children = getChildren(entity);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

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
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.75f, 1.0f, 1.0f));

            bool nodeOpen = ImGui::TreeNodeEx(nodeId.c_str(), flags);

            if (isPrefab)
                ImGui::PopStyleColor();

            if (!isRoot && ImGui::IsItemClicked())
                selectedEntity = entity;

            if (!isRoot && ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Entity"))
                {
                    scene.QueueDestroyEntity(entity);
                    if (selectedEntity == entity)
                        selectedEntity = entt::null;
                }
                ImGui::EndPopup();
            }

            if (!isRoot && ImGui::BeginDragDropSource())
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


    if (ImGui::BeginPopupContextWindow()) {
        if (ImGui::MenuItem("Create Entity")) {
            selectedEntity = scene.CreateEntity("New Entity");
        }
        
        if (ImGui::MenuItem("Camera")) {
            selectedEntity = scene.CreateEntity("Camera");
            scene.AddComponent<CameraComponent>(selectedEntity);
        }

        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Cube"))
                selectedEntity = LoadPrimitive(scene, "Cube");
            if (ImGui::MenuItem("Sphere"))
                selectedEntity = LoadPrimitive(scene, "Sphere");
            if (ImGui::MenuItem("Cylinder"))
                selectedEntity = LoadPrimitive(scene, "Cylinder");
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

        if (ImGui::MenuItem("Save as prefab"))
        {
            if (selectedEntity != entt::null)
            {
                const char* filterPatterns[] = { "*.rprefab" };
                const char* filePath = tinyfd_saveFileDialog(
                    "Create prefab as", "prefab.rprefab", 1, filterPatterns, NULL);

                if (filePath)
                {
                    std::filesystem::path prefabPath = filePath;

                    AssetRegistry& registry = AssetManager::Get().GetRegistry();
                    AssetMeta meta = registry.GetOrCreateMeta(prefabPath);

                    Ref<PrefabAsset> prefab = PrefabImporter::CreatePrefabAsset(
                        prefabPath, meta.uuid, scene, selectedEntity);

                    if (prefab)
                    {
                        meta.importerID = "PrefabImporter";
                        registry.SaveMeta(prefabPath, meta);
                        registry.Scan(registry.GetAssetDir());
                        LOG_INFO("Prefab saved: {}", prefabPath.string());
                    }
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

        ImGui::EndPopup();
    }

    ImGui::End();
}
