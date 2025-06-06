#pragma once
#include "Scene/Component.h"
#include "Core/Utils/FileUtils.h"
#include "../nlohmann/json.hpp"


struct MeshData
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int indexCount;
    std::string name;
    Path materialPath;
    uint16_t meshIndex;
    BoundingBox localAABB;
    glm::mat4 localTransform;
    glm::mat4 worldTransform;

    int GetTriangleCount()
    {
        return indexCount / 3;
    }

    void GetTriangle(int triangleIndex, glm::vec3& v0, glm::vec3& v1, glm::vec3& v2)
    {
        constexpr int stride = 8; 

        int i0 = indices[triangleIndex * 3 + 0];
        int i1 = indices[triangleIndex * 3 + 1];
        int i2 = indices[triangleIndex * 3 + 2];

        v0 = glm::vec3(
            vertices[i0 * stride + 0],
            vertices[i0 * stride + 1],
            vertices[i0 * stride + 2]
        );

        v1 = glm::vec3(
            vertices[i1 * stride + 0],
            vertices[i1 * stride + 1],
            vertices[i1 * stride + 2]
        );

        v2 = glm::vec3(
            vertices[i2 * stride + 0],
            vertices[i2 * stride + 1],
            vertices[i2 * stride + 2]
        );
    }
};

using json = nlohmann::json;

struct MeshComponent : public Component {
    Path modelPath;
    uint32_t meshIndex;
    MeshData mesh;

    MeshComponent() = delete;
    MeshComponent(const Path modelPath, uint16_t meshIndex, MeshData& mesh) : modelPath(modelPath), meshIndex(meshIndex), mesh(mesh) {}

    std::string Serialize() const override {
        json j;
        j["modelPath"] = modelPath.GetVirtualStr();
        j["meshIndex"] = meshIndex;
        return j.dump(4);
    }

    void Deserialize(const std::string& str) override {
        json j = json::parse(str);
        std::string modelPathData = j["modelPath"];
        modelPath = TO_ABSOLUTE_PATH(modelPathData);
        meshIndex = j["meshIndex"];
    }
};