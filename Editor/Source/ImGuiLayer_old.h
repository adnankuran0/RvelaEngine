#pragma once
#include "Core/Engine.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "Core/Layer.h"

class ImGuiLayer_old : public Layer
{
public:
    ImGuiLayer_old(Engine* engine) : m_Engine(engine) {}
    ~ImGuiLayer_old();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;
    void OnRender() override;
    void SetEditorCamera(EditorCamera* editorCamera) { m_EditorCamera = editorCamera; }
private:
    Engine* m_Engine;
    EditorCamera* m_EditorCamera;
    void DrawSceneHierarchyPanel(entt::registry& registry, entt::entity& selectedEntity);
    void DrawEntityCreationPanel(entt::registry& registry, entt::entity& selectedEntity);
    void DrawTransformEditor(entt::registry& registry, entt::entity selectedEntity);
    void DrawParentChildEditor(entt::registry& registry, entt::entity selectedEntity);
    void DrawInspectorPanel(entt::registry& registry, entt::entity& selectedEntity);
    void DrawCreateProjectPopup(Engine* engine);
    void DrawMenuBar();
};
