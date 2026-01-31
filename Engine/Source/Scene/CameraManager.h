#pragma once
#include <vector>
#include "Camera.h"
#include "Scene.h"

class CameraManager
{
public:
    CameraManager() = default;

    Camera CreateCamera(Scene& scene, const glm::vec3& position = { 0,0,0 });

    void SetActiveCamera(Camera& cam);

    Camera* GetActiveCamera() { return m_ActiveCamera; }

    void Update()
    {
        if (m_ActiveCamera)
            m_ActiveCamera->UpdateFrustum();
    }

    const std::vector<Camera>& GetAllCameras() const { return m_Cameras; }

private:
    std::vector<Camera> m_Cameras;
    Camera* m_ActiveCamera = nullptr;
};
