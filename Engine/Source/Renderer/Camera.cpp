#include "rvelapch.h"
#include "Camera.h"

using namespace rv;

glm::mat4 Camera::GetProjectionMatrix()
{
    float aspect = static_cast<float>(width) / static_cast<float>(height);
    if (ProjectionType == Projection::Perspective)
    {
        return glm::perspective(glm::radians(FOV), aspect, NearClip, FarClip);
    }
    else
    {
        float halfHeight = OrthoSize;
        float halfWidth = OrthoSize * aspect;

        return glm::ortho(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            NearClip, FarClip
        );
    }
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(Position, Position + Front, Up);
}

bool Camera::Intersects(const AABB& AABB)
{
    return frustum.Intersects(AABB);
}

bool Camera::Intersects(const glm::mat4& projView, const AABB& AABB)
{
    return frustum.Intersects(projView, AABB);
}

void Camera::UpdateFrustum()
{
    frustum.Update(GetProjectionMatrix() * GetViewMatrix());
}
