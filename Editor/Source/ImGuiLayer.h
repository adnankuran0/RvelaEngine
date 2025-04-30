#pragma once
#include "Core/Engine.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "Core/Layer.h"


class ImGuiLayer : public Layer
{
public:
    ImGuiLayer(Engine* engine) : m_Engine(engine) {}
    ~ImGuiLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;
    void OnRender() override;

private:
    Engine* m_Engine;
    void DrawSceneHierarchyPanel(entt::registry& registry, entt::entity& selectedEntity);
    void DrawEntityCreationPanel(entt::registry& registry, entt::entity& selectedEntity);
    void DrawTransformEditor(entt::registry& registry, entt::entity selectedEntity);
    void DrawParentChildEditor(entt::registry& registry, entt::entity selectedEntity);
    void DrawInspectorPanel(entt::registry& registry, entt::entity& selectedEntity);
    void DrawCreateProjectPopup(Engine* engine);
    void DrawMenuBar();
};
