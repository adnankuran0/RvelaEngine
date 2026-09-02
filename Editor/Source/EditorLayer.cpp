#include "EditorLayer.h"
#include "Scene/Entity.h"
#include "Core/Engine.h"
#include "GLFW/glfw3.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "Renderer/RenderLayer.h"
#include <Render/OutlinePass.h>
#include <Render/SelectedEntityMaskPass.h>
#include "Input/Input.h"
#include "EditorUtils.h"
#include "AssetImporters/PrefabImporter.h"
#include "AssetImporters/ModelImporter.h"
#include "AssetImporters/TextureImporter.h"
#include "AssetImporters/MeshImporter.h"
#include <memory>
#include "Event/Event.h"
#include "Event/MouseEvents.h"
#include <Event/WindowEvents.h>

using namespace rv;

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

    m_SelectedEntityMaskPass = m_Engine->GetRenderLayer().PushRenderPass(std::make_unique<SelectedEntityMaskPass>());
    m_OutlinePass = m_Engine->GetRenderLayer().PushRenderPass(std::make_unique<OutlinePass>());

    m_AssetImportPipeline.RegisterImporter(std::make_unique<ModelImporter>());
    m_AssetImportPipeline.RegisterImporter(std::make_unique<TextureImporter>());

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

    static_cast<SelectedEntityMaskPass*>(m_Engine->GetRenderLayer().GetRenderPass(m_SelectedEntityMaskPass))->SetSelectedEntity(m_SelectedEntity);
    static_cast<OutlinePass*>(m_Engine->GetRenderLayer().GetRenderPass(m_OutlinePass))->SetSelectedEntity(m_SelectedEntity);
   
    HandleShortcuts();
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

void EditorLayer::OnEvent(Event& event)
{
    switch (event.GetEventType())
    {
    case EventType::MouseMoved:
    {
        if (MouseMovedEvent* mouseEvent = static_cast<MouseMovedEvent*>(&event))
        {
            if (m_Engine->GetActiveScene().GetState() == SceneState::EDIT)
            {
                m_EditorCamera.OnMouseMoved(
                    mouseEvent->GetX(),
                    mouseEvent->GetY(),
                    m_Engine->GetWindow().GetGLFWWindow()
                );
            }

        }
        break;
    }
    case EventType::MouseScrolled:
    {
        if (Input::IsMouseButtonPressed(MouseCode::Button1))
        {
            if (MouseScrolledEvent* scrollEvent = static_cast<MouseScrolledEvent*>(&event))
            {
                // Adjust sprint speed based on scroll input
                m_EditorCamera.MovementSpeed += scrollEvent->GetYOffset();
                m_EditorCamera.MovementSpeed = std::clamp(m_EditorCamera.MovementSpeed, 1.0f, 100.0f);
            }
        }
        break;
    }
    case EventType::KeyPressed:
    {
        if (Input::IsKeyJustPressed(KeyCode::End))
        {
            LOG_DEBUG("Shaders reloaded.");
            ShaderManager::ReloadAll();
        }
        break;
    }
    case EventType::FileDropped:
    {
        if (FileDroppedEvent* fileEvent = static_cast<FileDroppedEvent*>(&event))
        {
            m_AssetBrowserPanel.HandleFileDrop(*fileEvent, m_AssetImportPipeline);
        }
        break;
    }
    default:
        break;
    }
}

void EditorLayer::Render()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    m_MenuBar.Draw(m_Engine,m_AssetImportPipeline);
     
    m_ToolBar.Draw(*m_Engine);

    m_Dockspace.Draw();

    m_SceneHierarchyPanel.Draw(m_Engine, m_SelectedEntity);

    m_InspectorPanel.Draw(m_Engine, m_SelectedEntity);

    m_EnvironmentPanel.Draw(m_Engine);

    m_AssetBrowserPanel.Draw(m_Engine, m_Engine->GetProjectManager().GetProjectPath() / "Assets",m_AssetImportPipeline);

    m_MixerPanel.Draw();

    m_AnimatorPanel.Draw(m_Engine, m_SelectedEntity);

    m_Viewport.Draw(m_Engine, m_SelectedEntity);

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

void EditorLayer::HandleShortcuts()
{
    if (Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl))
    {
        if (Input::IsKeyJustPressed(KeyCode::S))
        {
            EditorUtils::SaveScene(*m_Engine);
            m_Engine->GetProjectManager().SaveActiveProject();
        }
        if (Input::IsKeyJustPressed(KeyCode::O))
        {
            EditorUtils::OpenScene(*m_Engine);
        }
        if (Input::IsKeyJustPressed(KeyCode::N))
        {
            EditorUtils::CreateScene(*m_Engine);
        }

        // Ctrl + Shift 
        if (Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift))
        {
            if (Input::IsKeyJustPressed(KeyCode::S))
            {
                EditorUtils::SaveSceneAs(*m_Engine);
            }
        }

    }
}