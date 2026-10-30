#pragma once

#include <glm/glm.hpp>
#include <limits>

namespace rv {

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

    bool IsValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    bool Intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    AABB Transformed(const glm::mat4& transform) const {
        return CalculateWorldAABB(transform);
    }

    inline AABB CalculateWorldAABB(const glm::mat4& worldMatrix) const noexcept
    {
        if (!IsValid())
            return *this;

        glm::vec3 center = (min + max) * 0.5f;
        glm::vec3 extent = (max - min) * 0.5f;

        glm::vec3 newCenter = glm::vec3(worldMatrix * glm::vec4(center, 1.0f));

        glm::vec3 newExtent(
            glm::abs(worldMatrix[0][0]) * extent.x + glm::abs(worldMatrix[1][0]) * extent.y + glm::abs(worldMatrix[2][0]) * extent.z,
            glm::abs(worldMatrix[0][1]) * extent.x + glm::abs(worldMatrix[1][1]) * extent.y + glm::abs(worldMatrix[2][1]) * extent.z,
            glm::abs(worldMatrix[0][2]) * extent.x + glm::abs(worldMatrix[1][2]) * extent.y + glm::abs(worldMatrix[2][2]) * extent.z
        );

        return AABB(newCenter - newExtent, newCenter + newExtent);
    }
};

}
