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
    m_Engine->Run();
}


Engine* Editor::GetEngine()
{
    return m_Engine;
}


