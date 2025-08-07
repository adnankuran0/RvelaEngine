#pragma once
#include "Scene/Component.h"
#include "Utils/FileUtils.h"
#include "nlohmann/json.hpp"


struct alignas(16) MeshData
{
    BoundingBox localAABB;

    uint32_t indexCount;
    uint16_t meshIndex;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    Path materialPath;
    std::string name;

    int GetTriangleCount() const {
        return indexCount / 3;
    }

    void GetTriangle(int triangleIndex, glm::vec3& v0, glm::vec3& v1, glm::vec3& v2) const {
        constexpr int stride = 8;

        const int i0 = indices[triangleIndex * 3 + 0];
        const int i1 = indices[triangleIndex * 3 + 1];
        const int i2 = indices[triangleIndex * 3 + 2];

        v0 = glm::vec3(vertices[i0 * stride]);
        v1 = glm::vec3(vertices[i1 * stride]);
        v2 = glm::vec3(vertices[i2 * stride]);
    }
};

using json = nlohmann::json;

struct MeshComponent : public Component {
    MeshData mesh;
    Path modelPath;
    uint32_t meshIndex;

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
        modelPath = VRT_PATH(modelPathData);
        meshIndex = j["meshIndex"];
    }
};