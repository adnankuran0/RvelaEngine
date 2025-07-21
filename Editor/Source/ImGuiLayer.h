#pragma once
#include "Core/Engine.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "Core/Layer.h"
#include <vector>
#include <memory>
#include "GUI/SceneHierarchyPanel.h"

class ImGuiLayer : public Layer
{
public:
    ImGuiLayer(Engine* engine) : m_Engine(engine) 
    {
    }
    ~ImGuiLayer() {}

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate() override;
    void OnRender() override;
    void Render();
    void SetEditorCamera(EditorCamera* editorCamera) { m_EditorCamera = editorCamera; }

private:
    Engine* m_Engine;
    EditorCamera* m_EditorCamera;
};
