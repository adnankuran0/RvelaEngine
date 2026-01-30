#include "SceneHierarchyPanel.h"
#include "ImGui/imgui.h"
#include <ImGui/tinyfiledialogs.h>
#include "AssetImporter/PrefabImporter.h"


void SceneHierarchyPanel::Draw(Scene& scene, entt::entity& selectedEntity)
{
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
        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Cube"))
                selectedEntity = scene.LoadPrimitive("Cube");
            if (ImGui::MenuItem("Sphere"))
                selectedEntity = scene.LoadPrimitive("Sphere");
            if (ImGui::MenuItem("Cylinder"))
                selectedEntity = scene.LoadPrimitive("Cylinder");
            if (ImGui::MenuItem("Cone"))
                selectedEntity = scene.LoadPrimitive("Cone");
            if (ImGui::MenuItem("Capsule"))
                selectedEntity = scene.LoadPrimitive("Capsule");
            if (ImGui::MenuItem("Torus"))
                selectedEntity = scene.LoadPrimitive("Torus");
            if (ImGui::MenuItem("Plane"))
                selectedEntity = scene.LoadPrimitive("Plane");
            if (ImGui::MenuItem("Monkey"))
                selectedEntity = scene.LoadPrimitive("Monkey");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights")) {
            if (ImGui::MenuItem("Directional Light"))
            {
                selectedEntity = scene.CreateDirectionalLight();
            }
            if (ImGui::MenuItem("Point Light"))
            {
                selectedEntity = scene.CreatePointLight();
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