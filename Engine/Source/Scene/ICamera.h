#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include "AABB.h"
#include <Rendering/Frustum.h>

namespace rv {

class ICamera
{
public:
    virtual ~ICamera() = default;

    virtual glm::vec3 GetPosition() = 0;
    virtual glm::mat4 GetViewMatrix() = 0;
    virtual glm::mat4 GetProjectionMatrix() = 0;

    bool Intersects(const AABB& AABB)
    {
        return frustum.Intersects(AABB);
    }

    bool Intersects(const glm::mat4& projView, const AABB& AABB)
    {
        return frustum.Intersects(projView, AABB);
    }

    void UpdateFrustum()
    {
        frustum.Update(GetProjectionMatrix() * GetViewMatrix());
    }

protected:
    Frustum frustum;
    float width = 1920.0f;
    float height = 1080.0f;
};

}