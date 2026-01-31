#pragma once
#include "Scene/Components/CameraComponent.h"
#include "Scene/Components/TransformComponent.h"
#include <glm/gtc/matrix_transform.hpp>

class Entity; // forward declaration

class Camera
{
public:
    Camera(Entity* entity, float width = 1920.0f, float height = 1080.0f);
        

    glm::mat4 GetViewMatrix();
    

    glm::mat4 GetProjectionMatrix();
    

    void SetActive(bool active) { m_Component->isActive = active; }
    bool IsActive() const { return m_Component->isActive; }

    Entity* GetEntity() const { return m_Entity; }

private:
    float width, height;
    Entity* m_Entity;
    CameraComponent* m_Component;
    TransformComponent* m_Transform;
};
