#pragma once
#include <vector>
#include "Renderer/Camera.h"
#include "Core/Time.h"

namespace rv {

class Scene; // forward declaration

class CameraSystem
{
public:
    CameraSystem(Scene& scene) : m_Scene(scene) {}

    Camera* GetActiveCamera();

    void Update()
    {
        Camera* cam = GetActiveCamera();
        if (!cam) return;

        cam->UpdateFrustum();

        // update camera velocity
        if (cam->prevPosValid)
            cam->Velocity = (cam->Position - cam->prevPosition) / Time::GetDeltaTime();
        cam->prevPosition = cam->Position;
        cam->prevPosValid = true;
    }

private:
    Scene& m_Scene;

};

}