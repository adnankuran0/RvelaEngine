#include "Editor.h"
#include "EditorLayer.h"

Editor::Editor()
{
    rv::EditorLayer* editorLayer = new rv::EditorLayer(&m_Engine);
    m_Engine.SetEditorCamera(&editorLayer->GetEditorCamera());
    m_Engine.PushLayer(editorLayer);

}

void Editor::Run()
{
    m_Engine.Run();
}

