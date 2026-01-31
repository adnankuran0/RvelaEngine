#include "EditorLayer.h"
#include "Scene/Entity.h"


EditorLayer::~EditorLayer()
{
}

void EditorLayer::OnAttach()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    if (!m_Engine || !m_Engine->GetWindow().GetGLFWWindow())
    {
        LOG_ERROR("Error: Engine or Window is null!");
        return;
    }
    ImGui_ImplGlfw_InitForOpenGL(m_Engine->GetWindow().GetGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    SetStyle();
}

void EditorLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::OnUpdate()
{
    if(m_Engine->GetActiveScene().GetState() == SceneState::EDIT)
        m_EditorCamera.Update();

}

void EditorLayer::OnRender()
{
    Render();
}

void EditorLayer::OnFixedUpdate()
{
}

void EditorLayer::OnLateUpdate()
{
}

void EditorLayer::Render()
{
    static entt::entity selectedEntity = entt::null; // TODO: this is not belongs here

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    m_MenuBar.Draw(m_Engine,m_AssetImporterRegistry);

    m_ToolBar.Draw(m_Engine->GetActiveScene());

    m_Dockspace.Draw();

    m_SceneHierarchyPanel.Draw(m_Engine, selectedEntity);

    m_InspectorPanel.Draw(m_Engine, selectedEntity);

    m_AssetBrowserPanel.Draw(m_Engine, m_Engine->GetProjectManager().GetProjectPath() / "Assets");

    m_Viewport.Draw(m_Engine, selectedEntity);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}



