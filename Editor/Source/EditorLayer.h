#pragma once

#include "Core/Layer.h"
#include "Renderer/EditorCamera.h"
#include "Core/Engine.h"

class EditorLayer : public Layer
{
public:
    EditorLayer();
    ~EditorLayer();

    void OnAttach() override;
    void OnDetach() override;

    void OnUpdate() override;
    void OnFixedUpdate() override;
    void OnLateUpdate() override;

    EditorCamera& GetEditorCamera() { return m_EditorCamera; }

private:
    EditorCamera m_EditorCamera;
    bool m_ShowUI = true;
    bool m_ShowGrid = true;
};
