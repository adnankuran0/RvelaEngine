#include "SceneHierarchyPanel.h"
#include "ImGui/imgui.h"
#include <ImGui/tinyfiledialogs.h>

void SceneHierarchyPanel::Draw(Scene* scene,entt::entity& selectedEntity)
{

    entt::registry& registry = scene->GetRegistry();
    ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse);

    // Root entity'leri bul (parent'ı olmayanlar)
    auto rootEntities = [&registry]() {
        std::vector<entt::entity> roots;
        registry.view<SceneTreeComponent>().each([&](entt::entity e, SceneTreeComponent& stc) {
            if (stc.parent == entt::null) {
                roots.push_back(e);
            }
            });
        return roots;
        };

    // Bir entity'nin çocuklarını bulan fonksiyon
    auto getChildren = [&registry](entt::entity parent) {
        std::vector<entt::entity> children;
        registry.view<SceneTreeComponent>().each([&](entt::entity e, SceneTreeComponent& stc) {
            if (stc.parent == parent) {
                children.push_back(e);
            }
            });
        return children;
        };

    // Entity ağaç yapısını çizen rekürsif fonksiyon
    std::function<void(entt::entity)> DrawEntityNode = [&](entt::entity entity) {
        auto children = getChildren(entity);
        bool isLeaf = children.empty(); // Çocuk yoksa yaprak (leaf) düğüm
        ImGuiTreeNodeFlags flags = isLeaf ? ImGuiTreeNodeFlags_Leaf : 0;
        flags |= ImGuiTreeNodeFlags_OpenOnArrow; // Okla açma seçeneği

        // Eğer bu varlık seçiliyse, ImGuiTreeNodeFlags_Selected bayrağını ekle
        if (entity == selectedEntity) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        auto& tagComponent = scene->GetComponent<TagComponent>(entity);
        std::string nodeId = tagComponent.tag + "##" + std::to_string((uint32_t)entity);
        bool nodeOpen = ImGui::TreeNodeEx(nodeId.c_str(), flags);

        // Sol veya sağ tıkla entity seçimi
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            selectedEntity = entity;
        }

        // Sağ tıkla açılan entity'ye özel menü
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity")) {
                scene->DestroyEntity(entity);
                if (selectedEntity == entity) selectedEntity = entt::null;
            }
            ImGui::EndPopup();
        }

        // Çocukları varsa, açıldığında onları da çiz
        if (nodeOpen) {
            for (auto child : children) {
                DrawEntityNode(child);
            }
            ImGui::TreePop();
        }
        };

    // Tüm root entity'leri çiz
    for (auto root : rootEntities()) {
        DrawEntityNode(root);
    }

    if (ImGui::BeginPopupContextWindow()) {
        if (ImGui::MenuItem("Create Entity")) {
            selectedEntity = scene->CreateEntity("New Entity");
        }
        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Cube"))
                selectedEntity = scene->LoadPrimitive("Cube");
            if (ImGui::MenuItem("Sphere"))
                selectedEntity = scene->LoadPrimitive("Sphere");
            if (ImGui::MenuItem("Cylinder"))
                selectedEntity = scene->LoadPrimitive("Cylinder");
            if (ImGui::MenuItem("Cone"))
                selectedEntity = scene->LoadPrimitive("Cone");
            if (ImGui::MenuItem("Capsule"))
                selectedEntity = scene->LoadPrimitive("Capsule");
            if (ImGui::MenuItem("Torus"))
                selectedEntity = scene->LoadPrimitive("Torus");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights")) {
            if (ImGui::MenuItem("Directional Light"))
            {
                selectedEntity = scene->CreateDirectionalLight();
            }
            if (ImGui::MenuItem("Point Light"))
            {
                selectedEntity = scene->CreatePointLight();
            }
            if (ImGui::MenuItem("Spot Light"))
            {

            }
            if (ImGui::MenuItem("Area Light"))
            {

            }

            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Load Asset")) {
            const char* filterPatterns[] = { "*.fbx", "*.obj", "*.gltf", "*.glb" };
            const char* filePath = tinyfd_openFileDialog("Select a file", "", 4, filterPatterns, NULL, 0);

            std::string file = filePath ? std::string(filePath) : "";
            if (!file.empty())
            {
                selectedEntity = scene->LoadAsset(file);
            }
        }
        if (ImGui::MenuItem("Delete Entity")) {

            scene->DestroyEntity(selectedEntity);
            selectedEntity = entt::null;
        }
       /* if (ImGui::MenuItem("Create Project"))
        {
            createProjectRequested = true;
            strcpy(projectNameBuffer, "");
        }*/


        ImGui::EndPopup();
    }

    ImGui::End();
}