#pragma once
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace rv {

using json = nlohmann::json;

struct alignas(16) ReflectionProbeComponent
{
public:
    glm::vec3 extents = glm::vec3(5.0f);
    glm::vec3 originOffset = glm::vec3(0.0f);
    bool boxProjection = true;
    bool interior = false;
    float blendDistance = 0.5f;
    float intensity = 1.0f;
    int priority = 0;
    uint32_t resolution = 256;

    bool isDirty = true;
    int cubeIndex = -1; // -1 if not assigned

    ReflectionProbeComponent() = default;

    json Serialize() const;
    void Deserialize(const json& j);
};

}
