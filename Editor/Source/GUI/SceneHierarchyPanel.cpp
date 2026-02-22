#include "SceneHierarchyPanel.h"
#include "ImGui/imgui.h"
#include <ImGui/tinyfiledialogs.h>
#include "AssetImporter/PrefabImporter.h"
#include "Core/Engine.h"
#include "Scene/Entity.h"

namespace rv {

struct PrimitiveConfig { std::string name; AssetUUID uuid; };
static Entity LoadPrimitive(Scene& scene, const std::string& primitiveMeshName)
{
    static const std::unordered_map<std::string, PrimitiveConfig> map =
    {
        {"Cube", {"Cube", AssetUUID::FromString("36468d32-bba5-4eb4-82db-0d4da5cd6c65")}},
        {"Sphere", {"Sphere", AssetUUID::FromString("7f8ebf7d-a783-4a96-93e5-effcc76f557d")}},
        {"Cylinder", {"Cylinder", AssetUUID::FromString("dcb9be97-2538-4f80-8838-881c19d33ac4")}},
        {"Cone", {"Cone", AssetUUID::FromString("f91f0572-6cf8-4b72-adc0-18a17b7c05d8")}},
        {"Capsule", {"Capsule", AssetUUID::FromString("395a97e7-2cbe-4d65-95af-dea85f856252")}},
        {"Plane", {"Plane", AssetUUID::FromString("80da4157-714d-42a6-aedb-54eee61081f1")}},
        {"Monkey", {"Monkey", AssetUUID::FromString("e2e22656-e04b-4fb6-b7dd-70758a6c4762")}},
        {"Torus", {"Torus", AssetUUID::FromString("fe4c1c3d-4a95-4d2c-9883-bbf9c3e83530")}}
    };
    auto it = map.find(primitiveMeshName);
    if (it == map.end()) return Entity{};
    const auto& cfg = it->second;
    Entity root = scene.CreateEntity(cfg.name);
    Ref<MeshAsset> m = AssetRegistry::GetAsset<MeshAsset>(cfg.uuid);
    root.AddComponent<MeshRendererComponent>(m);
    root.AddComponent<MeshComponent>(m->GetUUID());
    root.GetComponent<TagComponent>().tag = cfg.name;
    AssetUUID defaultMaterialId = AssetUUID::FromString("ee3dde12-6263-4f11-bb1d-812b3e196ab7");
    root.AddComponent<MaterialComponent>(defaultMaterialId);
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
                    scene.DestroyEntity(entity);
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
            if (selectedEntity == entt::null) return;
            const char* filterPatterns[] = { "*.rprefab" };
            const char* filePath = tinyfd_saveFileDialog("Create prefab as", "prefab.rprefab", 1, filterPatterns, NULL);

            std::string file = filePath ? std::string(filePath) : "";
            if (!file.empty())
            {
                std::ofstream ofs(file);
                if (ofs.is_open())
                {
                    ofs.close();
                    PrefabImporter::CreatePrefabAsset(file, scene, selectedEntity);
                    AssetRegistry::ScanAssets();
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

}