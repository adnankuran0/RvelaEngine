#pragma once

#include "Core/Layer.h"
#include "Scene/EditorCamera.h"
#include "Core/Engine.h"
#include "AssetImporter/AssetImporterRegistry.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "GUI/SceneHierarchyPanel.h"
#include "GUI/Style.h"
#include "Scene/Components.h"
#include "Scene/Entity.h"
#define TINYFD_IMPLEMENTATION
#include "ImGui/tinyfiledialogs.h"
#include "Utils/Serializer.h"
#include "ImGui/ImGuizmo.h"
#include "GUI/MenuBar.h"
#include "GUI/ToolBar.h"
#include "GUI/Dockspace.h"
#include "GUI/Viewport.h"
#include "GUI/InspectorPanel.h"
#include "GUI/AssetBrowserPanel.h"
#include <ImGui/imgui_internal.h>

class EditorLayer : public Layer
{
public:
    EditorLayer(Engine* engine) : m_Engine(engine)
    {
    }
    ~EditorLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnFixedUpdate() override;
    void OnLateUpdate() override;
    void Render();

    EditorCamera& GetEditorCamera() { return m_EditorCamera; }

private:
    MenuBar m_MenuBar;
    ToolBar m_ToolBar;
    Dockspace m_Dockspace;
    SceneHierarchyPanel m_SceneHierarchyPanel;
    InspectorPanel m_InspectorPanel;
    AssetBrowserPanel m_AssetBrowserPanel;
    Viewport m_Viewport;

    AssetImporterRegistry m_AssetImporterRegistry;
    EditorCamera m_EditorCamera;
    Engine* m_Engine;
    bool m_ShowUI = true;
    bool m_ShowGrid = true;
};
