#pragma once

#include "Core/Layer.h"
#include "Renderer/EditorCamera.h"
#include "Asset/AssetImportPipeline.h"
#include "GUI/Style.h"
#include "GUI/MenuBar.h"
#include "GUI/ToolBar.h"
#include "GUI/Dockspace.h"
#include "GUI/Viewport.h"
#include "GUI/Panels/SceneHierarchyPanel.h"
#include "GUI/Panels/InspectorPanel.h"
#include "GUI/Panels/AssetBrowserPanel.h"
#include "GUI/Panels/MixerPanel.h"
#include "GUI/Panels/EnvironmentPanel.h"
#include "GUI/Panels/AnimatorPanel.h"
#include "GUI/Panels/ConsolePanel.h"
#include "GUI/Panels/ProjectSelectorPanel.h"
#include "GUI/Panels/ProjectSettingsPanel.h"
#include "GUI/Panels/ImportSettingsPanel.h"
#include "GUI/Panels/MaterialEditorPanel.h"
#include <Renderer/RenderPipeline.h>

namespace rv {

class Engine;

class EditorLayer : public Layer
{
public:
    EditorLayer(rv::Engine* engine);
    ~EditorLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnFixedUpdate() override;
    void OnLateUpdate() override;
    void OnEvent(Event& event) override;
    void Render();

    EditorCamera& GetEditorCamera() { return m_EditorCamera; }
    void ResetLayout();


private:
    void UpdateWindowMode();
    void HandleShortcuts();
    bool IsCursorOverViewport() const;

    MenuBar m_MenuBar;
    ToolBar m_ToolBar;
    Dockspace m_Dockspace;
    SceneHierarchyPanel m_SceneHierarchyPanel;
    InspectorPanel m_InspectorPanel;
    EnvironmentPanel m_EnvironmentPanel;
    AssetBrowserPanel m_AssetBrowserPanel;
    MixerPanel m_MixerPanel;
    AnimatorPanel m_AnimatorPanel;
    ConsolePanel m_ConsolePanel;
    ProjectSelectorPanel m_ProjectSelectorPanel;
    ProjectSettingsPanel m_ProjectSettingsPanel;
    ImportSettingsPanel m_ImportSettingsPanel;
    MaterialEditorPanel m_MaterialEditorPanel;
    Viewport m_Viewport;
    std::vector<EditorPanel*> m_DockPanels;

    RenderPassHandle m_GizmoPass;
    RenderPassHandle m_SelectedEntityMaskPass;
    RenderPassHandle m_OutlinePass;

    AssetImportPipeline m_AssetImportPipeline;
    EditorCamera m_EditorCamera;
    rv::Engine* m_Engine;
    bool m_ShowUI = true;
    bool m_ShowGrid = true;
    EditorCamera::NavigationMode m_EditorCameraNavigation = EditorCamera::NavigationMode::None;
    bool m_RightClickPending = false;
    glm::vec2 m_RightClickStartPos{ 0.0f, 0.0f };

    bool m_LauncherMode = true;
    bool m_SavedMaximized = true;
    int  m_SavedX = 100, m_SavedY = 100;
    int  m_SavedW = 1600, m_SavedH = 900;
    std::string m_LastWindowTitle;
};

}
