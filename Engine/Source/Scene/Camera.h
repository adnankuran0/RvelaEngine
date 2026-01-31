#pragma once
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/TransformComponent.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Scene/ICamera.h"

class Entity; // forward declaration

class Camera : public ICamera
{
public:
    Camera(Entity* entity);
        
    glm::mat4 GetProjectionMatrix() override;
    glm::mat4 GetViewMatrix() override;
    glm::vec3 GetPosition() override;
    
    void SetActive(bool active) { m_Component->isActive = active; }
    bool IsActive() const { return m_Component->isActive; }

    Entity* GetEntity() const { return m_Entity; }

private:
    Entity* m_Entity;
    CameraComponent* m_Component;
    TransformComponent* m_Transform;
};
