#include "rvelapch.h"
#include "CameraSystem.h"
#include "Entity.h"

using namespace rv;

Camera* CameraSystem::GetActiveCamera()
{
    for (auto& e : m_Scene.GetRegistry().view<CameraComponent>())
    {
        auto& camComp = m_Scene.GetComponent<CameraComponent>(e);
        if (camComp.isActive)
        {
            auto& camera = camComp.camera;
            auto& transformComp = m_Scene.GetComponent<TransformComponent>(e);
            camera.Position = transformComp.GetPosition();
            camera.Front = transformComp.GetForward();
            camera.Right = transformComp.GetRight();
            return &camera;
        }
    }

    return nullptr;
}
