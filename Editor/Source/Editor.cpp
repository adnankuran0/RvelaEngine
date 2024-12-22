#include "Editor.h"

Editor::Editor()
{
        s_Engine = new Engine();
        m_ImGuiLayer = new ImGuiLayer(s_Engine);
}

void Editor::Run()
{
    std::cout << "Engine has started!" << std::endl;
    m_ImGuiLayer->OnAttach();
    while (!glfwWindowShouldClose(s_Engine->GetWindow()->GetGLFWWindow()))
    {
        m_ImGuiLayer->OnUpdate();
        s_Engine->Run();
        m_ImGuiLayer->OnRender();
        glfwSwapBuffers(s_Engine->GetWindow()->GetGLFWWindow());
    }
    m_ImGuiLayer->OnDetach();
    std::cout << "Engine has stopped!" << std::endl;
}


Engine* Editor::GetEngine()
{
    return s_Engine;
}


