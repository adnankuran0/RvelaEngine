#include "ImGuiLayer.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"

//BUG: If you delete an entity other than the last one you created, it gives OPENGL 1282 error.

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

    ImGui::Begin("Engine Debug");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    if (ImGui::Button("Exit Engine")) {
        glfwSetWindowShouldClose(m_Engine->GetWindow()->GetGLFWWindow(), true);
    }
    ImGui::End();

    // Entity Management Panels
    static entt::entity selectedEntity = entt::null;

    DrawSceneHierarchyPanel(m_Engine->GetScene()->GetRegistry(), selectedEntity);
    DrawEntityCreationPanel(m_Engine->GetScene()->GetRegistry(), selectedEntity);
    DrawTransformEditor(m_Engine->GetScene()->GetRegistry(), selectedEntity);
    DrawParentChildEditor(m_Engine->GetScene()->GetRegistry(), selectedEntity);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::OnEvent()
{
    
}

void ImGuiLayer::DrawSceneHierarchyPanel(entt::registry& registry, entt::entity& selectedEntity) {
    ImGui::Begin("Scene Hierarchy");

    registry.view<TransformComponent>().each([&](entt::entity entity, TransformComponent&) {
        std::string entityName = "Entity " + std::to_string((uint32_t)entity);
        if (ImGui::Selectable(entityName.c_str(), selectedEntity == entity)) {
            selectedEntity = entity;
        }
        });

    ImGui::End();
}

void ImGuiLayer::DrawEntityCreationPanel(entt::registry& registry, entt::entity& selectedEntity) {
    ImGui::Begin("Entity Operations");

    if (ImGui::Button("Create Entity")) {
        selectedEntity = entt::entity(m_Engine->GetScene()->CreateEntity("test"));
        
    }

    if (selectedEntity != entt::null) {
        if (ImGui::Button("Delete Selected Entity")) {
            m_Engine->GetScene()->DestroyEntity(selectedEntity);
            selectedEntity = entt::null;
        }
    }

    ImGui::End();
}

void ImGuiLayer::DrawTransformEditor(entt::registry& registry, entt::entity selectedEntity) {
    if (selectedEntity == entt::null || !registry.any_of<TransformComponent>(selectedEntity))
        return;

    auto& transform = registry.get<TransformComponent>(selectedEntity);

    ImGui::Begin("Transform Editor");

    ImGui::Text("Position");
    ImGui::DragFloat3("##Position", &transform.position[0], 0.1f);

    ImGui::Text("Rotation");
    ImGui::DragFloat3("##Rotation", &transform.rotation[0], 0.1f);

    ImGui::Text("Scale");
    ImGui::DragFloat3("##Scale", &transform.scale[0], 0.1f, 0.1f, 10.0f);

    ImGui::End();
}

void ImGuiLayer::DrawParentChildEditor(entt::registry& registry, entt::entity selectedEntity) {
    if (selectedEntity == entt::null || !registry.any_of<SceneTreeComponent>(selectedEntity))
        return;

    auto& selectedNode = registry.get<SceneTreeComponent>(selectedEntity);

    ImGui::Begin("Parent/Child Editor");

    ImGui::Text("Parent");

    if (ImGui::BeginCombo("##Parent", selectedNode.parent == entt::null ? "None" : std::to_string((uint32_t)selectedNode.parent).c_str())) {
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

    ImGui::End();
}
