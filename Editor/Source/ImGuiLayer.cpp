#include "ImGuiLayer.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"
#define TINYFD_IMPLEMENTATION
#include "ImGui/tinyfiledialogs.h"
#include "Utils/Serializer.h"
#include "ImGui/ImGuizmo.h"
#include "MenuBar.h"
#include "ToolBar.h"
#include "Dockspace.h"
#include "Viewport.h"
#include "SceneHierarchyPanel.h"
#include "InspectorPanel.h"
#include "AssetBrowserPanel.h"
#include "Style.h"
#include <ImGui/imgui_internal.h>

void ImGuiLayer::OnAttach()
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

    if (!m_Engine || !m_Engine->GetWindow())
    {
        std::cerr << "Error: Engine or Window is null!" << std::endl;
        return;
    }
    ImGui_ImplGlfw_InitForOpenGL(m_Engine->GetWindow()->GetGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    SetStyle();
}

void ImGuiLayer::OnDetach()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::OnUpdate()
{
}

void ImGuiLayer::OnRender()
{
    Render();
}

void ImGuiLayer::Render()
{
    static entt::entity selectedEntity = entt::null; // TODO: this is not belongs here

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    MenuBar::Draw(m_Engine);
    
    ToolBar::Draw();
   
    Dockspace::Draw();
    
    SceneHierarchyPanel::Draw(m_Engine->GetScene(), selectedEntity);

    InspectorPanel::Draw(m_Engine->GetScene(), selectedEntity);

    AssetBrowserPanel::Draw(m_Engine,m_Engine->GetProjectManager()->GetProjectPath() / "Assets");

    Viewport::Draw(m_Engine,selectedEntity);

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
