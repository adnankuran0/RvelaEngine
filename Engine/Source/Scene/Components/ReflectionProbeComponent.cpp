#include "rvelapch.h"
#include "ReflectionProbeComponent.h"

using namespace rv;

json ReflectionProbeComponent::Serialize() const
{
    json j;
    j["extents"] = { extents.x, extents.y, extents.z };
    j["originOffset"] = { originOffset.x, originOffset.y, originOffset.z };
    j["boxProjection"] = boxProjection;
    j["interior"] = interior;
    j["blendDistance"] = blendDistance;
    j["intensity"] = intensity;
    j["priority"] = priority;
    j["resolution"] = resolution;
    return j;
}

void ReflectionProbeComponent::Deserialize(const json& j)
{
    if (j.contains("extents"))
    {
        auto e = j.at("extents");
        extents = glm::vec3(e[0], e[1], e[2]);
    }
    if (j.contains("originOffset"))
    {
        auto o = j.at("originOffset");
        originOffset = glm::vec3(o[0], o[1], o[2]);
    }
    if (j.contains("boxProjection")) boxProjection = j.at("boxProjection").get<bool>();
    if (j.contains("interior")) interior = j.at("interior").get<bool>();
    if (j.contains("blendDistance")) blendDistance = j.at("blendDistance").get<float>();
    if (j.contains("intensity")) intensity = j.at("intensity").get<float>();
    if (j.contains("priority")) priority = j.at("priority").get<int>();
    if (j.contains("resolution")) resolution = j.at("resolution").get<uint32_t>();
    isDirty = true;
    cubeIndex = -1;
}
