#pragma once
#include <Jolt/Jolt.h>
#include "../nlohmann/json.hpp"

namespace rv {

using json = nlohmann::json;

struct BoxColliderComponent
{
public:

    glm::vec3 GetSize() { return size; }
    void SetSize(const glm::vec3& newSize) { size = newSize; isDirty = true; }
    glm::vec3 GetOffset() { return offset; }
    void SetOffset(const glm::vec3& newOffset) { offset = newOffset; isDirty = true; }

    bool IsDirty() { return isDirty; }
    void ClearDirty() { isDirty = false; }

    json Serialize() const;
    void Deserialize(const json& j);

private:
    bool isDirty = false;
    glm::vec3 size = { 0.5f, 0.5f, 0.5f };
    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
};

struct SphereColliderComponent 
{
public:
    float GetRadius() { return radius; }
    void SetRadius(float newRadius) { radius = newRadius; isDirty = true; }
    glm::vec3 GetOffset() { return offset; }
    void SetOffset(const glm::vec3& newOffset) { offset = newOffset; isDirty = true; }

    bool IsDirty() { return isDirty; }
    void ClearDirty() { isDirty = false; }

    json Serialize() const;
    void Deserialize(const json& j);
private:
    bool isDirty = false;
    float radius = 0.5f;
    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
    
  
};

struct CapsuleColliderComponent 
{
public:
    float GetRadius() { return radius; }
    void SetRadius(float newRadius) { radius = newRadius; isDirty = true; }
    float GetHalfHeight() { return halfHeight; }
    void SetHalfHeight(float newHalfHeight) { halfHeight = newHalfHeight; isDirty = true; }
    glm::vec3 GetOffset() { return offset; }
    void SetOffset(const glm::vec3& newOffset) { offset = newOffset; isDirty = true; }

    bool IsDirty() { return isDirty; }
    void ClearDirty() { isDirty = false; }

    json Serialize() const;
    void Deserialize(const json& j);
private:
    bool isDirty = false;

    float radius = 0.5f;
    float halfHeight = 0.5f;
    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };

   
};

struct CylinderColliderComponent
{
public:
    float GetRadius() { return radius; }
    void SetRadius(float newRadius) { radius = newRadius; isDirty = true; }
    float GetHalfHeight() { return halfHeight; }
    void SetHalfHeight(float newHalfHeight) { halfHeight = newHalfHeight; isDirty = true; }
    glm::vec3 GetOffset() { return offset; }
    void SetOffset(const glm::vec3& newOffset) { offset = newOffset; isDirty = true; }

    bool IsDirty() { return isDirty; }
    void ClearDirty() { isDirty = false; }

    json Serialize() const;
    void Deserialize(const json& j);
private:
    bool isDirty = false;

    float radius = 0.5f;
    float halfHeight = 0.5f;
    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };
};

struct MeshColliderComponent
{
public:
    int GetMaxTrianglesPerLeaf() { return maxTrianglesPerLeaf; }
    void SetMaxTrianglesPerLeaf(unsigned int maxTrisPerLeaf) { maxTrianglesPerLeaf = maxTrisPerLeaf; isDirty = true; }
    float GetActiveEdgeTresholdAngle() { return activeEdgeTresholdAngle; }
    void SetActiveEdgeTresholdAngle(float activeEdgeTresholdAng) { activeEdgeTresholdAngle = activeEdgeTresholdAng; isDirty = true; }
    glm::vec3 GetOffset() { return offset; }
    void SetOffset(const glm::vec3& newOffset) { offset = newOffset; isDirty = true; }

    bool IsDirty() { return isDirty; }
    void ClearDirty() { isDirty = false; }

    json Serialize() const;
    void Deserialize(const json& j);
private:
    bool isDirty = false;

    int maxTrianglesPerLeaf = 8;
    float activeEdgeTresholdAngle = 0.996195f;

    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };

   
};

struct ConvexHullColliderComponent
{
public:
    float GetMaxConvexRadius() { return maxConvexRadius; }
    void SetMaxConvexRadius(float maxConvexRad) { maxConvexRadius = maxConvexRad; isDirty = true; }
    glm::vec3 GetOffset() { return offset; }
    void SetOffset(const glm::vec3& newOffset) { offset = newOffset; isDirty = true; }

    bool IsDirty() { return isDirty; }
    void ClearDirty() { isDirty = false; }

    json Serialize() const;
    void Deserialize(const json& j);
private:
    bool isDirty = false;

    float maxConvexRadius = 0.0f;
    glm::vec3 offset = { 0.0f, 0.0f, 0.0f };

  
};

}