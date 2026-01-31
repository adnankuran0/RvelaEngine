#include "rvelapch.h"
#include "Camera.h"
#include "Scene/Entity.h"

Camera::Camera(Entity* entity, float width = 1920.0f, float height = 1080.0f)
{
    m_Entity = entity;
    m_Component = &entity->GetComponent<CameraComponent>();
    m_Transform = &entity->GetComponent<TransformComponent>();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(
        m_Transform->GetPosition(),
        m_Transform->GetPosition() + m_Transform->GetForward(),
        m_Transform->GetUp()
    );
}

glm::mat4 Camera::GetProjectionMatrix()
{
    return glm::perspective(glm::radians(m_Component->fov), width / height, m_Component->nearClip, m_Component->farClip);
}