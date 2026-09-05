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
#include <Renderer/RenderPipeline.h>

namespace rv {

class Engine;

class EditorLayer : public Layer
{
public:
    EditorLayer(rv::Engine* engine) : m_Engine(engine)
    {
    }
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


private:
    void HandleShortcuts();

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
    Viewport m_Viewport;

    RenderPassHandle m_OutlinePass;
    RenderPassHandle m_SelectedEntityMaskPass;

    std::vector<entt::entity> m_SelectedEntities;

    entt::entity m_SelectedEntity = entt::null;
    AssetImportPipeline m_AssetImportPipeline;
    EditorCamera m_EditorCamera;
    rv::Engine* m_Engine;
    bool m_ShowUI = true;
    bool m_ShowGrid = true;
};

}