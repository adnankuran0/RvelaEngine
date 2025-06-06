#include "Editor.h"
#include "EditorLayer.h"
#include "ImGuiLayer.h"

Editor::Editor()
{
    m_Engine = new Engine();

    EditorLayer* editorLayer = new EditorLayer();

    m_Engine->SetEditorCamera(&editorLayer->GetEditorCamera());

    m_Engine->PushLayer(editorLayer);
    ImGuiLayer* imGuiLayer = new ImGuiLayer(m_Engine);
    imGuiLayer->SetEditorCamera(&editorLayer->GetEditorCamera());
    m_Engine->PushLayer(imGuiLayer);
}

void Editor::Run()
{
    m_Engine->Run();
}


Engine* Editor::GetEngine()
{
    return m_Engine;
}


