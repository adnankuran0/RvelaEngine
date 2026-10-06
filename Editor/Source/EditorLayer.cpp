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
#include <Render/GizmoPass.h>
#include "Input/Input.h"
#include "EditorUtils.h"
#include "Scene/Components/TransformComponent.h"
#include "AssetImporters/PrefabImporter.h"
#include "AssetImporters/ModelImporter.h"
#include "AssetImporters/TextureImporter.h"
#include "AssetImporters/MeshImporter.h"
#include <memory>
#include "Event/Event.h"
#include "Event/MouseEvents.h"
#include <Event/WindowEvents.h>
#include <Render/IconLibrary.h>
#include "EditorSettings.h"
#include "EditorSelection.h"

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

    m_GizmoPass = m_Engine->GetRenderLayer().PushRenderPass(std::make_unique<GizmoPass>());
    m_SelectedEntityMaskPass = m_Engine->GetRenderLayer().PushRenderPass(std::make_unique<SelectedEntityMaskPass>());
    m_OutlinePass = m_Engine->GetRenderLayer().PushRenderPass(std::make_unique<OutlinePass>());

    m_AssetImportPipeline.RegisterImporter(std::make_unique<ModelImporter>());
    m_AssetImportPipeline.RegisterImporter(std::make_unique<TextureImporter>());

    IconLibrary::Init();

    EditorSettings::Get().Load();
    EditorSettings::Get().ApplyToCamera(m_EditorCamera);
    EditorSettings::Get().ApplyToViewport(m_Viewport);
}

void EditorLayer::OnDetach()
{
    EditorSettings::Get().UpdateFromCamera(m_EditorCamera);
    EditorSettings::Get().UpdateFromViewport(m_Viewport);
    EditorSettings::Get().Save();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorLayer::OnUpdate()
{
    if (m_Engine->GetActiveScene().GetState() == SceneState::EDIT)
        m_EditorCamera.Update(m_EditorCameraDragActive);

    auto& reg = m_Engine->GetActiveScene().GetRegistry();
    EditorSelection::Get().Validate(reg);

    static_cast<SelectedEntityMaskPass*>(m_Engine->GetRenderLayer().GetRenderPass(m_SelectedEntityMaskPass))->SetSelectedEntities(EditorSelection::Get().GetSelectedEntities());
    static_cast<OutlinePass*>(m_Engine->GetRenderLayer().GetRenderPass(m_OutlinePass))->SetHasSelection(!EditorSelection::Get().IsEmpty());
   
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

bool EditorLayer::IsCursorOverViewport() const
{
    GLFWwindow* window = m_Engine->GetWindow().GetGLFWWindow();
    if (!window)
        return false;

    double cursorX = 0.0;
    double cursorY = 0.0;
    int windowX = 0;
    int windowY = 0;
    glfwGetCursorPos(window, &cursorX, &cursorY);
    glfwGetWindowPos(window, &windowX, &windowY);

    return m_Viewport.ContainsPoint(glm::vec2(
        static_cast<float>(cursorX + windowX),
        static_cast<float>(cursorY + windowY)));
}

void EditorLayer::OnEvent(Event& event)
{
    switch (event.GetEventType())
    {
    case EventType::MouseButtonPressed:
    {
        if (m_Engine->GetActiveScene().GetState() == SceneState::EDIT)
        {
            if (auto* mouseEvent = static_cast<MouseButtonPressedEvent*>(&event);
                mouseEvent->GetMouseCode() == MouseCode::ButtonRight)
            {
                m_EditorCameraDragActive = IsCursorOverViewport();
                if (m_EditorCameraDragActive)
                    event.Handled = true;
            }
        }
        if (m_Engine->GetActiveScene().GetState() == SceneState::PLAY && !Input::IsMouseCaptured())
        {
            if (!IsCursorOverViewport())
                Input::SetGameplayInputEnabled(false);
        }
        break;
    }
    case EventType::MouseButtonReleased:
    {
        if (m_Engine->GetActiveScene().GetState() == SceneState::EDIT)
        {
            if (auto* mouseEvent = static_cast<MouseButtonReleasedEvent*>(&event);
                mouseEvent->GetMouseCode() == MouseCode::ButtonRight)
            {
                m_EditorCameraDragActive = false;
                m_EditorCamera.EndMouseCapture(m_Engine->GetWindow().GetGLFWWindow());
                event.Handled = true;
            }
        }
        break;
    }
    case EventType::MouseMoved:
    {
        if (MouseMovedEvent* mouseEvent = static_cast<MouseMovedEvent*>(&event))
        {
            if (m_Engine->GetActiveScene().GetState() == SceneState::EDIT && m_EditorCameraDragActive)
            {
                m_EditorCamera.OnMouseMoved(
                    mouseEvent->GetX(),
                    mouseEvent->GetY(),
                    m_Engine->GetWindow().GetGLFWWindow()
                );
                event.Handled = true;
            }

        }
        break;
    }
    case EventType::MouseScrolled:
    {
        if (MouseScrolledEvent* scrollEvent = static_cast<MouseScrolledEvent*>(&event))
        {
            if (IsCursorOverViewport())
            {
                m_EditorCamera.ProcessMouseScroll(scrollEvent->GetYOffset());
                event.Handled = true;
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
    ImGuiIO& io = ImGui::GetIO();
    const bool gameplayOwnsInput = m_Engine->GetActiveScene().GetState() == SceneState::PLAY && Input::IsGameplayInputEnabled();
    if (gameplayOwnsInput)
        io.ConfigFlags |= ImGuiConfigFlags_NoKeyboard;
    else
        io.ConfigFlags &= ~ImGuiConfigFlags_NoKeyboard;

    if ((gameplayOwnsInput && Input::IsMouseCaptured()) || m_EditorCameraDragActive)
        io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    else
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    bool projectSelected = m_ProjectSelectorPanel.Draw(m_Engine);
    if (projectSelected)
    {
        EditorSelection::Get().Clear();
    }

    if (ProjectManager::IsProjectLoaded())
    {
        m_MenuBar.Draw(m_Engine, m_AssetImportPipeline, &m_ProjectSettingsPanel, &m_ProjectSelectorPanel);
         
        m_ToolBar.Draw(*m_Engine);

        m_Dockspace.Draw();

        m_SceneHierarchyPanel.Draw(m_Engine);

        m_InspectorPanel.Draw(m_Engine);

        m_EnvironmentPanel.Draw(m_Engine);

        m_AssetBrowserPanel.Draw(m_Engine, ProjectManager::GetAssetDirectory(), m_AssetImportPipeline);

        m_MixerPanel.Draw();

        m_AnimatorPanel.Draw(m_Engine);

        m_ConsolePanel.Draw();

        m_ProjectSettingsPanel.Draw(m_Engine);

        m_Viewport.Draw(m_Engine);
    }

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
    if (m_Engine->GetActiveScene().GetState() == SceneState::PLAY && Input::IsGameplayInputEnabled())
        return;

    if (ImGui::GetIO().WantTextInput)
        return;

    bool isCtrl = Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl);
    bool isShift = Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift);

    if (isCtrl)
    {
        if (isShift)
        {
            if (Input::IsKeyJustPressed(KeyCode::S))
            {
                EditorUtils::SaveSceneAs(*m_Engine);
            }
        }
        else
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
            if (Input::IsKeyJustPressed(KeyCode::D))
            {
                if (!EditorSelection::Get().IsEmpty())
                {
                    auto& scene = m_Engine->GetActiveScene();
                    auto& reg = scene.GetRegistry();
                    auto rootEntity = scene.GetRootEntity();
                    const auto& selected = EditorSelection::Get().GetSelectedEntities();
                    entt::entity primary = EditorSelection::Get().GetPrimary();

                    auto isDescendantOfAny = [&](entt::entity e, const std::vector<entt::entity>& entityList) -> bool {
                        entt::entity cur = e;
                        while (cur != entt::null && reg.valid(cur) && scene.HasComponent<SceneTreeComponent>(cur)) {
                            entt::entity parent = scene.GetComponent<SceneTreeComponent>(cur).parent;
                            if (parent == entt::null || parent == rootEntity) break;
                            if (std::find(entityList.begin(), entityList.end(), parent) != entityList.end()) return true;
                            cur = parent;
                        }
                        return false;
                    };

                    std::vector<entt::entity> newSelection;
                    entt::entity newPrimary = entt::null;
                    for (auto e : selected)
                    {
                        if (e != entt::null && reg.valid(e) && !isDescendantOfAny(e, selected))
                        {
                            Entity duplicated = scene.DuplicateEntity(e);
                            if (duplicated.GetHandle() != entt::null)
                            {
                                newSelection.push_back(duplicated.GetHandle());
                                if (e == primary)
                                    newPrimary = duplicated.GetHandle();
                            }
                        }
                    }

                    if (!newSelection.empty())
                    {
                        EditorSelection::Get().SetSelection(newSelection, newPrimary);
                    }
                }
            }
        }
    }
    else
    {
        if (Input::IsKeyJustPressed(KeyCode::Delete))
        {
            auto& scene = m_Engine->GetActiveScene();
            auto& reg = scene.GetRegistry();
            for (auto e : EditorSelection::Get().GetSelectedEntities())
            {
                if (e != entt::null && reg.valid(e))
                {
                    scene.QueueDestroyEntity(e);
                }
            }
            EditorSelection::Get().Clear();
        }

        if (Input::IsKeyJustPressed(KeyCode::F))
        {
            if (!EditorSelection::Get().IsEmpty())
            {
                auto& scene = m_Engine->GetActiveScene();
                auto& reg = scene.GetRegistry();
                glm::vec3 minBound(FLT_MAX);
                glm::vec3 maxBound(-FLT_MAX);
                bool hasBounds = false;

                for (auto e : EditorSelection::Get().GetSelectedEntities())
                {
                    if (e != entt::null && reg.valid(e) && reg.any_of<TransformComponent>(e))
                    {
                        auto& tc = reg.get<TransformComponent>(e);
                        glm::vec3 pos = tc.GetWorldPosition();
                        glm::vec3 scale = glm::max(glm::abs(tc.GetWorldScale()), glm::vec3(0.5f));
                        minBound = glm::min(minBound, pos - scale * 0.5f);
                        maxBound = glm::max(maxBound, pos + scale * 0.5f);
                        hasBounds = true;
                    }
                }

                if (hasBounds)
                {
                    glm::vec3 center = (minBound + maxBound) * 0.5f;
                    float radius = glm::length(maxBound - minBound) * 0.5f;
                    float distance = std::clamp(radius * 2.5f + 1.0f, 3.0f, 100.0f);
                    m_EditorCamera.Focus(center, distance);
                }
            }
        }

        if (Input::IsKeyJustPressed(KeyCode::Escape))
        {
            EditorSelection::Get().Clear();
        }
    }
}
