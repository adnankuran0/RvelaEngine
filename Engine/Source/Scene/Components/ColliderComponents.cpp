#include "rvelapch.h"
#include "ColliderComponents.h"

using namespace rv;

json BoxColliderComponent::Serialize() const
{
    json j;
    j["size"] = { size.x, size.y, size.z };
    j["offset"] = { offset.x, offset.y, offset.z };
    return j;
}

void BoxColliderComponent::Deserialize(const json& j)
{
    auto sizeData = j.at("size");
    size = glm::vec3(sizeData[0], sizeData[1], sizeData[2]);
    auto offsetData = j.at("offset");
    offset = glm::vec3(offsetData[0], offsetData[1], offsetData[2]);
}

json SphereColliderComponent::Serialize() const
{
    json j;
    j["radius"] = radius;
    j["offset"] = { offset.x, offset.y, offset.z };
    return j;
}

void SphereColliderComponent::Deserialize(const json& j)
{
    radius = j.at("radius").get<float>();
    auto offsetData = j.at("offset");
    offset = glm::vec3(offsetData[0], offsetData[1], offsetData[2]);
}

json CapsuleColliderComponent::Serialize() const
{
    json j;
    j["radius"] = radius;
    j["halfHeight"] = halfHeight;
    j["offset"] = { offset.x, offset.y, offset.z };
    return j;
}

void CapsuleColliderComponent::Deserialize(const json& j)
{
    radius = j.at("radius").get<float>();
    halfHeight = j.at("halfHeight").get<float>();
    auto offsetData = j.at("offset");
    offset = glm::vec3(offsetData[0], offsetData[1], offsetData[2]);
}

json CylinderColliderComponent::Serialize() const
{
    json j;
    j["radius"] = radius;
    j["halfHeight"] = halfHeight;
    j["offset"] = { offset.x, offset.y, offset.z };
    return j;
}

void CylinderColliderComponent::Deserialize(const json& j)
{
    radius = j.at("radius").get<float>();
    halfHeight = j.at("halfHeight").get<float>();
    auto offsetData = j.at("offset");
    offset = glm::vec3(offsetData[0], offsetData[1], offsetData[2]);
}

json MeshColliderComponent::Serialize() const
{
    json j;
    j["maxTrianglesPerLeaf"] = maxTrianglesPerLeaf;
    j["activeEdgeTresholdAngle"] = activeEdgeTresholdAngle;
    j["offset"] = { offset.x, offset.y, offset.z };
    return j;
}

void MeshColliderComponent::Deserialize(const json& j)
{
    maxTrianglesPerLeaf = j.at("maxTrianglesPerLeaf").get<int>();
    activeEdgeTresholdAngle = j.at("activeEdgeTresholdAngle").get<float>();
    auto offsetData = j.at("offset");
    offset = glm::vec3(offsetData[0], offsetData[1], offsetData[2]);
}

json ConvexHullColliderComponent::Serialize() const
{
    json j;
    j["maxConvexRadius"] = maxConvexRadius;
    j["offset"] = { offset.x, offset.y, offset.z };
    return j;
}

void ConvexHullColliderComponent::Deserialize(const json& j)
{
    maxConvexRadius = j.at("maxConvexRadius").get<float>();
    auto offsetData = j.at("offset");
    offset = glm::vec3(offsetData[0], offsetData[1], offsetData[2]);
}