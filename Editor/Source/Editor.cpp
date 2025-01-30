#include "Editor.h"


Editor::Editor()
{
        m_Engine = new Engine();
        m_ImGuiLayer = new ImGuiLayer(m_Engine);
}

void Editor::Run()
{
    std::cout << "Engine has started!" << std::endl;
    m_ImGuiLayer->OnAttach();
    while (!glfwWindowShouldClose(m_Engine->GetWindow()->GetGLFWWindow()))
    {
        Time::update();
        m_ImGuiLayer->OnUpdate();
        m_Engine->Run();
        m_ImGuiLayer->OnRender();
        glfwSwapBuffers(m_Engine->GetWindow()->GetGLFWWindow());
    }
    m_ImGuiLayer->OnDetach();
    std::cout << "Engine has stopped!" << std::endl;
}


Engine* Editor::GetEngine()
{
    return m_Engine;
}


