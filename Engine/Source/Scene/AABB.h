#pragma once

#include <glm/glm.hpp>
#include <limits>

class AABB {
public:
    glm::vec3 min;
    glm::vec3 max;

    AABB() {
        Reset();
    }

    AABB(const glm::vec3& min, const glm::vec3& max)
        : min(min), max(max) {
    }

    void Reset() {
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(-std::numeric_limits<float>::max());
    }

    void Expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    glm::vec3 Center() const {
        return (min + max) * 0.5f;
    }

    glm::vec3 Size() const {
        return max - min;
    }

    bool Intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    AABB Transformed(const glm::mat4& transform) const {
        glm::vec3 corners[8] = {
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {min.x, max.y, min.z},
            {max.x, max.y, min.z},
            {min.x, min.y, max.z},
            {max.x, min.y, max.z},
            {min.x, max.y, max.z},
            {max.x, max.y, max.z},
        };

        AABB result;
        for (int i = 0; i < 8; ++i) {
            glm::vec4 transformedCorner = transform * glm::vec4(corners[i], 1.0f);
            result.Expand(glm::vec3(transformedCorner));
        }

        return result;
    }

    inline AABB CalculateWorldAABB(const glm::mat4& worldMatrix) noexcept
    {
        static constexpr glm::vec3 corners[8] = 
        {
            {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
            {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}
        };

        glm::vec3 worldMin(std::numeric_limits<float>::max());
        glm::vec3 worldMax(std::numeric_limits<float>::lowest());

        for (int i = 0; i < 8; ++i) {
            glm::vec3 corner = glm::mix(min, max, corners[i]); 
            glm::vec4 worldPos = worldMatrix * glm::vec4(corner, 1.0f);
            glm::vec3 p(worldPos);

            worldMin = glm::min(worldMin, p);
            worldMax = glm::max(worldMax, p);
        }

        return AABB(worldMin, worldMax);
    }
};
