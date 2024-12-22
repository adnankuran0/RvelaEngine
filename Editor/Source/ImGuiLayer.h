#pragma once
#include "Core/Engine.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"

class ImGuiLayer
{
public:
    ImGuiLayer(Engine* engine);
    ~ImGuiLayer();

    void OnAttach();
    void OnDetach();
    void OnUpdate();
    void OnRender();
    void OnEvent();

private:
    Engine* m_Engine;
};
