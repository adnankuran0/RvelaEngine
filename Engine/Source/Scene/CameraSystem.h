#pragma once
#include <vector>
#include "Camera.h"

class Scene; // forward declaration

class CameraSystem
{
public:
    CameraSystem() = default;

    Camera& CreateCamera(Scene& scene, const glm::vec3& position = { 0,0,0 });

    void SetActiveCamera(Camera& cam);

    Camera* GetActiveCamera() { return m_ActiveCamera; }

    void Update()
    {
        if (m_ActiveCamera)
            m_ActiveCamera->UpdateFrustum();
    }

    const std::vector<std::unique_ptr<Camera>>& GetAllCameras() const { return m_Cameras; }

private:
    std::vector<std::unique_ptr<Camera>> m_Cameras;
    Camera* m_ActiveCamera = nullptr;
};
