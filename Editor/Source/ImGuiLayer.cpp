#include "ImGuiLayer.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"
#define TINYFD_IMPLEMENTATION
#include "ImGui/tinyfiledialogs.h"

std::string OpenFileDialog()
{
    const char* filterPatterns[] = { "*.fbx", "*.obj", "*.gltf"};
    const char* filePath = tinyfd_openFileDialog("Select a file", "", 3, filterPatterns, NULL, 0);

    return filePath ? std::string(filePath) : "";
}

ImGuiLayer::ImGuiLayer(Engine* engine)
    : m_Engine(engine)
{
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

    ImGui::Begin("Engine Debug");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    if (ImGui::Button("Exit Engine")) {
        glfwSetWindowShouldClose(m_Engine->GetWindow()->GetGLFWWindow(), true);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::OnEvent()
{
    // Olayları işleyin (gerekirse)
}

void ImGuiLayer::DrawSceneHierarchyPanel(entt::registry& registry, entt::entity& selectedEntity)
{
    ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

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

      

        ImGui::EndPopup();
    }

    ImGui::End();
}

void ImGuiLayer::DrawInspectorPanel(entt::registry& registry, entt::entity& selectedEntity)
{
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

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

        if (registry.any_of<TransformComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& transform = registry.get<TransformComponent>(selectedEntity);
                ImGui::DragFloat3("Position", &transform.position[0], 0.1f);
                ImGui::DragFloat3("Rotation", &transform.rotation[0], 0.1f, -360.0f, 360.0f);
                ImGui::DragFloat3("Scale", &transform.scale[0], 0.1f, 0.1f, 10.0f);
                if (ImGui::Button("Reset Transform")) {
                    transform.position = { 0.0f, 0.0f, 0.0f };
                    transform.rotation = { 0.0f, 0.0f, 0.0f };
                    transform.scale = { 1.0f, 1.0f, 1.0f };
                }
            }
        }

        if (registry.any_of<SceneTreeComponent>(selectedEntity)) {
            if (ImGui::CollapsingHeader("Parent/Child", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& selectedNode = registry.get<SceneTreeComponent>(selectedEntity);
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
            }
        }
    }
    else {
        ImGui::Text("No entity selected.");
    }

    ImGui::End();
}