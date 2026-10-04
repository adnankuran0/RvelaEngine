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
            m_EditorCamera.Update();

    auto& reg = m_Engine->GetActiveScene().GetRegistry();
    m_SelectedEntities.erase(
        std::remove_if(m_SelectedEntities.begin(), m_SelectedEntities.end(),
            [&reg](entt::entity e) { return e == entt::null || !reg.valid(e); }),
        m_SelectedEntities.end()
    );
    if (m_SelectedEntity != entt::null && !reg.valid(m_SelectedEntity))
    {
        m_SelectedEntity = m_SelectedEntities.empty() ? entt::null : m_SelectedEntities.back();
    }
    m_Engine->GetActiveScene().SetSelectedEntity(m_SelectedEntity);

    static_cast<SelectedEntityMaskPass*>(m_Engine->GetRenderLayer().GetRenderPass(m_SelectedEntityMaskPass))->SetSelectedEntities(m_SelectedEntities);
    static_cast<OutlinePass*>(m_Engine->GetRenderLayer().GetRenderPass(m_OutlinePass))->SetHasSelection(!m_SelectedEntities.empty());
   
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
        if (MouseScrolledEvent* scrollEvent = static_cast<MouseScrolledEvent*>(&event))
        {
            m_EditorCamera.ProcessMouseScroll(scrollEvent->GetYOffset());
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

    bool projectSelected = m_ProjectSelectorPanel.Draw(m_Engine);
    if (projectSelected)
    {
        m_SelectedEntity = entt::null;
        m_SelectedEntities.clear();
    }

    if (ProjectManager::IsProjectLoaded())
    {
        m_MenuBar.Draw(m_Engine, m_AssetImportPipeline, &m_ProjectSettingsPanel, &m_ProjectSelectorPanel);
         
        m_ToolBar.Draw(*m_Engine);

        m_Dockspace.Draw();

        m_SceneHierarchyPanel.Draw(m_Engine, m_SelectedEntity, m_SelectedEntities);

        m_InspectorPanel.Draw(m_Engine, m_SelectedEntity, m_SelectedEntities);

        m_EnvironmentPanel.Draw(m_Engine);

        m_AssetBrowserPanel.Draw(m_Engine, ProjectManager::GetAssetDirectory(), m_AssetImportPipeline);

        m_MixerPanel.Draw();

        m_AnimatorPanel.Draw(m_Engine, m_SelectedEntity);

        m_ConsolePanel.Draw();

        m_ProjectSettingsPanel.Draw(m_Engine);

        m_Viewport.Draw(m_Engine, m_SelectedEntity, m_SelectedEntities);
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
                if (!m_SelectedEntities.empty())
                {
                    auto& scene = m_Engine->GetActiveScene();
                    auto& reg = scene.GetRegistry();
                    auto rootEntity = scene.GetRootEntity();

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
                    for (auto e : m_SelectedEntities)
                    {
                        if (e != entt::null && reg.valid(e) && !isDescendantOfAny(e, m_SelectedEntities))
                        {
                            Entity duplicated = scene.DuplicateEntity(e);
                            if (duplicated.GetHandle() != entt::null)
                            {
                                newSelection.push_back(duplicated.GetHandle());
                                if (e == m_SelectedEntity)
                                    newPrimary = duplicated.GetHandle();
                            }
                        }
                    }

                    if (!newSelection.empty())
                    {
                        m_SelectedEntities = newSelection;
                        m_SelectedEntity = (newPrimary != entt::null) ? newPrimary : newSelection.back();
                        scene.SetSelectedEntity(m_SelectedEntity);
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
            for (auto e : m_SelectedEntities)
            {
                if (e != entt::null && reg.valid(e))
                {
                    scene.QueueDestroyEntity(e);
                }
            }
            m_SelectedEntity = entt::null;
            m_SelectedEntities.clear();
            scene.SetSelectedEntity(entt::null);
        }

        if (Input::IsKeyJustPressed(KeyCode::F))
        {
            if (!m_SelectedEntities.empty())
            {
                auto& scene = m_Engine->GetActiveScene();
                auto& reg = scene.GetRegistry();
                glm::vec3 minBound(FLT_MAX);
                glm::vec3 maxBound(-FLT_MAX);
                bool hasBounds = false;

                for (auto e : m_SelectedEntities)
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
            m_SelectedEntity = entt::null;
            m_SelectedEntities.clear();
            m_Engine->GetActiveScene().SetSelectedEntity(entt::null);
        }
    }
}