#include "EditorLayer.h"
#include "Scene/Entity.h"

EditorLayer::EditorLayer()
    : Layer("EditorLayer"), m_EditorCamera(glm::vec3(0.0f, 2.0f, 10.0f))
{
}

EditorLayer::~EditorLayer()
{
}

void EditorLayer::OnAttach()
{
}

void EditorLayer::OnDetach()
{
}

void EditorLayer::OnUpdate()
{
    m_EditorCamera.Update();

}

void EditorLayer::OnFixedUpdate()
{
}

void EditorLayer::OnLateUpdate()
{
}


