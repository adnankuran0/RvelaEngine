#include "rvelapch.h"
#include "CameraManager.h"
#include "Entity.h"

Camera CameraManager::CreateCamera(Scene& scene, const glm::vec3& position)
{
    Entity camEntity = scene.CreateEntity("Camera");
    camEntity.AddComponent<CameraComponent>();
    auto& transform = camEntity.GetComponent<TransformComponent>();
    transform.SetPosition(position);

    Camera cam(&camEntity);
    m_Cameras.push_back(cam);

    if (!m_ActiveCamera)
        SetActiveCamera(cam);

    return cam;
}

void CameraManager::SetActiveCamera(Camera& cam)
{
    for (auto& c : m_Cameras)
        c.SetActive(false);

    cam.SetActive(true);
    m_ActiveCamera = &cam;
}