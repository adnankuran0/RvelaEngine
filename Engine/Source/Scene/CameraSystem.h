#pragma once
#include <vector>
#include "Camera.h"

namespace rv {

class Scene; // forward declaration

class CameraSystem
{
public:
    CameraSystem(Scene& scene) : m_Scene(scene) {}

    Camera* GetActiveCamera();

    void Update()
    {
        if (GetActiveCamera())
            GetActiveCamera()->UpdateFrustum();
    }

private:
    Scene& m_Scene;

};

}