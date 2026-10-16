#include "Editor.h"
#include "EditorLayer.h"

Editor::Editor()
    : m_Engine(rv::WindowData("Rvela Engine", 820, 540, false))
{
    rv::EditorLayer* editorLayer = new rv::EditorLayer(&m_Engine);
    m_Engine.SetEditorCamera(&editorLayer->GetEditorCamera());
    m_Engine.PushLayer(editorLayer);

}

void Editor::Run()
{
    m_Engine.Run();
}

