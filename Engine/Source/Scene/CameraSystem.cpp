#include "rvelapch.h"
#include "CameraSystem.h"
#include "Entity.h"

Camera& CameraSystem::CreateCamera(Scene& scene, const glm::vec3& position)
{
    Entity camEntity = scene.CreateEntity("Camera");
    camEntity.AddComponent<CameraComponent>();
    auto& transform = camEntity.GetComponent<TransformComponent>();
    transform.SetPosition(position);

    auto cam = std::make_unique<Camera>(&camEntity);
    Camera& camRef = *cam;
    m_Cameras.push_back(std::move(cam));

    if (!m_ActiveCamera)
        SetActiveCamera(camRef);

    return camRef;
}

void CameraSystem::SetActiveCamera(Camera& cam)
{
    for (auto& c : m_Cameras)
        c->SetActive(false);

    cam.SetActive(true);
    m_ActiveCamera = &cam;
}