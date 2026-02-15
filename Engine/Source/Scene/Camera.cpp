#include "rvelapch.h"
#include "Camera.h"
#include "Scene/Entity.h"

namespace rv {

Camera::Camera(Entity* entity)
{
    m_Entity = entity;
    m_Component = &entity->GetComponent<CameraComponent>();
    m_Transform = &entity->GetComponent<TransformComponent>();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(
        m_Transform->GetWorldPosition(),
        m_Transform->GetWorldPosition() + m_Transform->GetForward(),
        m_Transform->GetUp()
    );
}

glm::vec3 Camera::GetPosition()
{
    return m_Transform->GetWorldPosition();
}

glm::mat4 Camera::GetProjectionMatrix()
{
    return glm::perspective(glm::radians(m_Component->fov), width / height, m_Component->nearClip, m_Component->farClip);
}

}