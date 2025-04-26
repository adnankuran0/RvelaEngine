#include "Editor.h"
#include "ImGuiLayer.h"
#include "EditorLayer.h"

Editor::Editor()
{
    m_Engine = new Engine();

    EditorLayer* editorLayer = new EditorLayer();

    m_Engine->SetEditorCamera(&editorLayer->GetEditorCamera());

    m_Engine->PushLayer(editorLayer);
    Layer* imGuiLayer = new ImGuiLayer(m_Engine);
    m_Engine->PushLayer(imGuiLayer);
}

void Editor::Run()
{
    std::cout << "Engine has started!" << std::endl;
    m_Engine->Run();
    std::cout << "Engine has stopped!" << std::endl;
}


Engine* Editor::GetEngine()
{
    return m_Engine;
}


