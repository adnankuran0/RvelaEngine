#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include "AABB.h"
#include <Rendering/Frustum.h>

namespace rv {



class Camera
{
public:

    enum class Projection
    {
        Perspective,
        Orthographic
    };

    virtual ~Camera() = default;

    glm::mat4 GetProjectionMatrix()
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

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

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

    Frustum frustum;
    glm::vec3 Position{};
    glm::vec3 Front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 Up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 Right{1.0f,0.0f,0.0f};
    glm::vec3 WorldUp{ 0.0f, 1.0f, 0.0f};
    float FOV{ 90.0f };
    float OrthoSize{ 10.0f };
    float NearClip{ 0.1f };
    float FarClip{ 1000.0f };
    int width{ 1920 };
    int height{ 1080 };
    Projection ProjectionType = Projection::Perspective;
};

}