#include "ImGuiLayer.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"
#define TINYFD_IMPLEMENTATION
#include "ImGui/tinyfiledialogs.h"
#include "Core/Utils/Serializer.h"


static bool createProjectRequested = false;
static char projectNameBuffer[128] = "";



void SetModernDarkImGuiStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.14f, 0.75f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.25f, 0.29f, 0.60f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.23f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.29f, 0.32f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.27f, 0.29f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.17f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.15f, 0.21f, 0.25f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.30f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.30f, 0.55f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.TabRounding = 4.0f;
}

std::string OpenFileDialog()
{
    const char* filterPatterns[] = { "*.fbx", "*.obj", "*.gltf"};
    const char* filePath = tinyfd_openFileDialog("Select a file", "", 3, filterPatterns, NULL, 0);

    return filePath ? std::string(filePath) : "";
}

std::string OpenFolderDialog()
{
    const char* folderPath = tinyfd_selectFolderDialog("Select a Folder", "");

    return folderPath ? std::string(folderPath) : "";
}

ImGuiLayer::~ImGuiLayer()
{
}

void ImGuiLayer::OnAttach()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Docking özelliğini kontrol et ve etkinleştir
#ifdef ImGuiConfigFlags_DockingEnable
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    std::cout << "Docking özelliği etkinleştirildi." << std::endl;
#else
#endif

    ImGui_ImplGlfw_InitForOpenGL(m_Engine->GetWindow()->GetGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    ImGui::StyleColorsDark();
}

void ImGuiLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::OnUpdate()
{
    SetModernDarkImGuiStyle();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::OnRender()
{
    static entt::entity selectedEntity = entt::null;

    // Docking destekleniyorsa dockspace oluştur
#ifdef ImGuiConfigFlags_DockingEnable
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#else
    // Docking yoksa manuel sabitleme
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * 0.2f, ImGui::GetIO().DisplaySize.y));
#endif

    DrawSceneHierarchyPanel(m_Engine->GetScene()->GetRegistry(), selectedEntity);

#ifdef ImGuiConfigFlags_DockingEnable
    // Docking varsa sağa sabitle
#else
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.8f, 0));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x * 0.2f, ImGui::GetIO().DisplaySize.y));
#endif
    DrawInspectorPanel(m_Engine->GetScene()->GetRegistry(), selectedEntity);
    DrawCreateProjectPopup(m_Engine);


    ImGui::Begin("Engine Debug");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    if (ImGui::Button("Exit Engine")) {
        glfwSetWindowShouldClose(m_Engine->GetWindow()->GetGLFWWindow(), true);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::DrawCreateProjectPopup(Engine* engine)
{
    if (createProjectRequested)
    {
        ImGui::Begin("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Project Name:");
        ImGui::InputText("##projectname", projectNameBuffer, IM_ARRAYSIZE(projectNameBuffer));

        if (ImGui::Button("Create")) {
            std::string folder = OpenFolderDialog();
            if (!folder.empty()) {
                engine->GetProjectManager()->CreateProject(projectNameBuffer, folder);
                createProjectRequested = false;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            createProjectRequested = false;
        }

        ImGui::End();
    }
}



void ImGuiLayer::DrawSceneHierarchyPanel(entt::registry& registry, entt::entity& selectedEntity)
{
    ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

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

        auto& tagComponent = m_Engine->GetScene()->GetComponent<TagComponent>(entity);
        std::string entityName = tagComponent.tag;
        bool nodeOpen = ImGui::TreeNodeEx(entityName.c_str(), flags);

        // Sol veya sağ tıkla entity seçimi
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            selectedEntity = entity;
        }

        // Sağ tıkla açılan entity'ye özel menü
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity")) {
                m_Engine->GetScene()->DestroyEntity(entity);
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

    // Panelin boş alanına sağ tıkla yeni entity oluşturma menüsü
    if (ImGui::BeginPopupContextWindow()) {
        if (ImGui::MenuItem("Create Entity")) {
            selectedEntity = m_Engine->GetScene()->CreateEntity("New Entity");
        }
        if (ImGui::BeginMenu("Primitives")) {
            if (ImGui::MenuItem("Cube"))
                selectedEntity = m_Engine->GetScene()->LoadPrimitive("Cube");
            if (ImGui::MenuItem("Sphere"))
                selectedEntity = m_Engine->GetScene()->LoadPrimitive("Sphere");
            if (ImGui::MenuItem("Cylinder"))
                selectedEntity = m_Engine->GetScene()->LoadPrimitive("Cylinder");
            if (ImGui::MenuItem("Cone"))
                selectedEntity = m_Engine->GetScene()->LoadPrimitive("Cone");
            if (ImGui::MenuItem("Capsule"))
                selectedEntity = m_Engine->GetScene()->LoadPrimitive("Capsule");
            if (ImGui::MenuItem("Torus"))
                selectedEntity = m_Engine->GetScene()->LoadPrimitive("Torus");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lights")) {
            if (ImGui::MenuItem("Directional Light"))
            {
                selectedEntity = m_Engine->GetScene()->CreateDirectionalLight();
            }
            if (ImGui::MenuItem("Point Light"))
            {
                selectedEntity = m_Engine->GetScene()->CreatePointLight();
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
            std::string file = OpenFileDialog();
            if (!file.empty())
            {
                selectedEntity = m_Engine->GetScene()->LoadAsset(file);
            }
        }
        if (ImGui::MenuItem("Delete Entity")) {
            
            m_Engine->GetScene()->DestroyEntity(selectedEntity);
            selectedEntity = entt::null;
        }
        if (ImGui::MenuItem("Save Scene"))
        {
            m_Engine->GetSceneManager()->SaveScene(*m_Engine->GetScene(), "D:\\GitHub\\RvelaEngine\\TestProject\\Scenes\\Test.rscene");
        }
        if (ImGui::MenuItem("Load Scene"))
        {
            m_Engine->GetSceneManager()->LoadScene(*m_Engine->GetScene(), "D:\\GitHub\\RvelaEngine\\TestProject\\Scenes\\Test.rscene");
        }
        if (ImGui::MenuItem("Create Project"))
        {
            createProjectRequested = true;
            strcpy(projectNameBuffer, "");
        }
      

        ImGui::EndPopup();
    }

    ImGui::End();
}

void ImGuiLayer::DrawInspectorPanel(entt::registry& registry, entt::entity& selectedEntity)
{
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    if (selectedEntity != entt::null) {
        ImGui::Text("Selected Entity: %u", (uint32_t)selectedEntity);

        // Component'leri alt alta ve açılıp kapanabilir şekilde göster

        if (registry.any_of<TagComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Name Tag",ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& tag = registry.get<TagComponent>(selectedEntity);

                static char buffer[256]; // Geçici bir buffer
                std::strncpy(buffer, tag.tag.c_str(), sizeof(buffer)); // Başlangıçta std::string içeriğini kopyala

                if (ImGui::InputText("##hidden", buffer, sizeof(buffer))) {
                    if (buffer != "")
                        tag.tag = buffer; // Kullanıcı değişiklik yaptığında string'i güncelle
                }
            }
        }

        if (registry.any_of<UUIDComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("UUID")) {
                auto& uuid = registry.get<UUIDComponent>(selectedEntity);
                ImGui::Text("UUID: %s", std::to_string(uuid.uuid).c_str());
            }
        }

        if (registry.any_of<TransformComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& transform = registry.get<TransformComponent>(selectedEntity);


                glm::vec3 euler = transform.GetEulerRotation();
                bool isScaleRatioLocked = transform.IsScaleRatioLocked();

                glm::vec3 pos = transform.GetPosition();
                if (ImGui::DragFloat3("Position", &pos[0], 0.1f)) {
                    transform.SetPosition(pos);
                }

                if (ImGui::DragFloat3("Rotation", &euler[0], 0.1f, -360, 360)) {
                    transform.SetEulerRotation(euler);
                }

                ImGui::PushID("ScaleControls");
                glm::vec3 scale = transform.GetScale();
                if(ImGui::DragFloat3("Scale", &scale[0], 0.1f, 0.01f, 10.0f))
                {
                    transform.SetScale(scale);
                }
                if(ImGui::Checkbox("Lock Ratio", &isScaleRatioLocked))
                {
                    transform.SetLockScaleRatio(isScaleRatioLocked);
                }
                ImGui::PopID();

                


                if (ImGui::Button("Reset Transform")) {
                    transform.SetPosition({ 0.0f, 0.0f, 0.0f });
                    transform.SetEulerRotation({ 0.0f, 0.0f, 0.0f });
                    transform.SetScale({1.0f, 1.0f, 1.0f});
                }
            }
        }

        if (registry.any_of<MeshComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& meshComponent = registry.get<MeshComponent>(selectedEntity);
                ImGui::Text("Model Path: %s", meshComponent.modelPath.GetAbsoluteStr().c_str());
                ImGui::Text("Mesh Index: %s", std::to_string(meshComponent.meshIndex));
            }
        }

        if (registry.any_of<PointLightComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("PointLight", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& pointLightComponent = registry.get<PointLightComponent>(selectedEntity);
                glm::vec3 lightColor = pointLightComponent.color;
                float color[3] = { lightColor.r, lightColor.g, lightColor.b };
                if (ImGui::ColorPicker3("Light Color", color)) {
                    pointLightComponent.color = glm::vec3(color[0], color[1], color[2]);

                }

                if (ImGui::SliderFloat("Intensity", &pointLightComponent.intensity, 0.0f, 10.0f)) {}
                if (ImGui::SliderFloat("Radius", &pointLightComponent.radius, 0.1f, 50.0f)) {}

            }
        }

        if (registry.any_of<DirectionalLightComponent>(selectedEntity))
        {
            if (ImGui::CollapsingHeader("DirectionalLight", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& directionalLightComponent = registry.get<DirectionalLightComponent>(selectedEntity);
                glm::vec3 lightColor = directionalLightComponent.color;
                float color[3] = { lightColor.r, lightColor.g, lightColor.b };
                if (ImGui::ColorPicker3("Light Color", color)) {
                    directionalLightComponent.color = glm::vec3(color[0], color[1], color[2]);

                }

                if (ImGui::SliderFloat("Intensity", &directionalLightComponent.intensity, 0.0f, 10.0f)) {}
                if (ImGui::Checkbox("Cast Shadow", &directionalLightComponent.castShadows));

            }
        }

        if (registry.any_of<MaterialComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                MaterialComponent& materialComponent = registry.get<MaterialComponent>(selectedEntity);
                const ISerializable* materialSerializable = std::static_pointer_cast<const ISerializable>(materialComponent.material).get();

                ImGui::Text("Material Path: %s", materialComponent.GetMaterialPath().GetAbsoluteStr().c_str());
            
                glm::vec3 albedoColor = materialComponent.material->albedoColor;
                float color[3] = { albedoColor.r, albedoColor.g, albedoColor.b };

                if (ImGui::ColorPicker3("Albedo", color)) {
                    materialComponent.material->albedoColor = glm::vec3(color[0], color[1], color[2]);
                    
                    Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

                if (ImGui::SliderFloat("AO", &materialComponent.material->ao, 0.0f, 1.0f)) {
                    Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

                if (ImGui::SliderFloat("Metallic", &materialComponent.material->metallic, 0.0f, 1.0f)) {
                    Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

                if (ImGui::SliderFloat("Roughness", &materialComponent.material->roughness, 0.0f, 1.0f)) {
                    Serializer::SaveToFile(*materialSerializable, materialComponent.GetMaterialPath().GetAbsoluteStr());
                }

            }
        }

        if (registry.any_of<SceneTreeComponent>(selectedEntity)) {
            auto& selectedNode = registry.get<SceneTreeComponent>(selectedEntity);
            if (ImGui::CollapsingHeader("Parent/Child", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::BeginCombo("Parent", selectedNode.parent == entt::null ? "None" : std::to_string((uint32_t)selectedNode.parent).c_str())) {
                    if (ImGui::Selectable("None", selectedNode.parent == entt::null)) {
                        m_Engine->GetScene()->RemoveParent(selectedEntity);
                    }
                    registry.view<SceneTreeComponent>().each([&](entt::entity entity, SceneTreeComponent&) {
                        if (entity != selectedEntity) {
                            if (ImGui::Selectable(std::to_string((uint32_t)entity).c_str(), selectedNode.parent == entity)) {
                                m_Engine->GetScene()->SetParent(selectedEntity, entity);
                            }
                        }
                        });
                    ImGui::EndCombo();
                }
                auto& uuid = selectedNode.parentUUID;
                ImGui::Text("Parent UUID: %s", std::to_string(uuid).c_str());
                ImGui::Text("Children UUIDs:");
                for (const auto& childUUID : selectedNode.childrenUUIDs) {
                    ImGui::Text("- %s", std::to_string(childUUID).c_str());
                }
            }
        }

    }
    else {
        ImGui::Text("No entity selected.");
    }

    ImGui::End();
}