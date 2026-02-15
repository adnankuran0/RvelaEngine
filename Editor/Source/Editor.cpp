#include "Editor.h"
#include "EditorLayer.h"
#include "Core/Engine.h"


Editor::Editor()
{
    m_Engine = new rv::Engine();

    rv::EditorLayer* editorLayer = new rv::EditorLayer(m_Engine);

    m_Engine->SetEditorCamera(&editorLayer->GetEditorCamera());

    m_Engine->PushLayer(editorLayer);

}

void Editor::Run()
{
    m_Engine->Run();
}


rv::Engine* Editor::GetEngine()
{
    return m_Engine;
}

