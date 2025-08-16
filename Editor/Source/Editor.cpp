#include "Editor.h"
#include "EditorLayer.h"

Editor::Editor()
{
    m_Engine = new Engine();

    EditorLayer* editorLayer = new EditorLayer(m_Engine);

    m_Engine->SetEditorCamera(&editorLayer->GetEditorCamera());

    m_Engine->PushLayer(editorLayer);
}

void Editor::Run()
{
    m_Engine->Run();
}


Engine* Editor::GetEngine()
{
    return m_Engine;
}


