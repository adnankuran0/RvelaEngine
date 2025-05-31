#pragma once

#include <glm/glm.hpp>
#include <array>
#include <Scene/BoundingBox.h>
class Frustum
{
public:
    enum Planes
    {
        Near = 0,
        Far,
        Left,
        Right,
        Top,
        Bottom,
        Count
    };

    std::array<glm::vec4, Count> planes;

    
    void Update(const glm::mat4& projView)
    {
        planes[Left] = glm::vec4(projView[0][3] + projView[0][0], projView[1][3] + projView[1][0], projView[2][3] + projView[2][0], projView[3][3] + projView[3][0]);
        planes[Right] = glm::vec4(projView[0][3] - projView[0][0], projView[1][3] - projView[1][0], projView[2][3] - projView[2][0], projView[3][3] - projView[3][0]);
        planes[Bottom] = glm::vec4(projView[0][3] + projView[0][1], projView[1][3] + projView[1][1], projView[2][3] + projView[2][1], projView[3][3] + projView[3][1]);
        planes[Top] = glm::vec4(projView[0][3] - projView[0][1], projView[1][3] - projView[1][1], projView[2][3] - projView[2][1], projView[3][3] - projView[3][1]);
        planes[Near] = glm::vec4(projView[0][3] + projView[0][2], projView[1][3] + projView[1][2], projView[2][3] + projView[2][2], projView[3][3] + projView[3][2]);
        planes[Far] = glm::vec4(projView[0][3] - projView[0][2], projView[1][3] - projView[1][2], projView[2][3] - projView[2][2], projView[3][3] - projView[3][2]);

        for (auto& p : planes)
        {
            float length = glm::length(glm::vec3(p));
            p /= length;
        }
    }

    bool Intersects(const BoundingBox& AABB) const
    {
        for (const auto& plane : planes)
        {
            glm::vec3 normal = glm::vec3(plane);
            glm::vec3 positiveVertex = AABB.min;

            if (normal.x >= 0)
                positiveVertex.x = AABB.max.x;
            if (normal.y >= 0)
                positiveVertex.y = AABB.max.y;
            if (normal.z >= 0)
                positiveVertex.z = AABB.max.z;

            if (glm::dot(normal, positiveVertex) + plane.w < 0)
                return false; 
        }
        return true; 
    }

    bool Intersects(const glm::mat4& projView, const BoundingBox& AABB) const
    {
        std::array<glm::vec4, Count> planes;

        planes[Left] = glm::vec4(projView[0][3] + projView[0][0], projView[1][3] + projView[1][0], projView[2][3] + projView[2][0], projView[3][3] + projView[3][0]);
        planes[Right] = glm::vec4(projView[0][3] - projView[0][0], projView[1][3] - projView[1][0], projView[2][3] - projView[2][0], projView[3][3] - projView[3][0]);
        planes[Bottom] = glm::vec4(projView[0][3] + projView[0][1], projView[1][3] + projView[1][1], projView[2][3] + projView[2][1], projView[3][3] + projView[3][1]);
        planes[Top] = glm::vec4(projView[0][3] - projView[0][1], projView[1][3] - projView[1][1], projView[2][3] - projView[2][1], projView[3][3] - projView[3][1]);
        planes[Near] = glm::vec4(projView[0][3] + projView[0][2], projView[1][3] + projView[1][2], projView[2][3] + projView[2][2], projView[3][3] + projView[3][2]);
        planes[Far] = glm::vec4(projView[0][3] - projView[0][2], projView[1][3] - projView[1][2], projView[2][3] - projView[2][2], projView[3][3] - projView[3][2]);

        for (auto& p : planes)
        {
            float length = glm::length(glm::vec3(p));
            p /= length;
        }

        for (const auto& plane : planes)
        {
            glm::vec3 normal = glm::vec3(plane);
            glm::vec3 positiveVertex = AABB.min;

            if (normal.x >= 0)
                positiveVertex.x = AABB.max.x;
            if (normal.y >= 0)
                positiveVertex.y = AABB.max.y;
            if (normal.z >= 0)
                positiveVertex.z = AABB.max.z;

            if (glm::dot(normal, positiveVertex) + plane.w < 0)
                return false;
        }
        return true;
    }


};
