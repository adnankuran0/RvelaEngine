#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Assets/Asset.h"
#include "Assets/AssetMeta.h"
#include "Rendering/AABB.h"
#include <span>
#include <glm/glm.hpp>

namespace rv { 

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec2 texCoord;
};

void PackVertices(std::span<const Vertex> verts, std::vector<float>& out);

class MeshMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override;
    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override;
    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override;

public:
    unsigned int vertexCount = 0;
    unsigned int indexCount = 0;
};

class MeshAsset : public Asset
{
public:
    MeshAsset(const std::string& path, std::unique_ptr<MeshMeta> meshMeta);
    ~MeshAsset();

    bool Load();
    bool IsLoaded() const;

    int GetTriangleCount() const;
    void GetTriangle(int triangleIndex,
        glm::vec3& v0,
        glm::vec3& v1,
        glm::vec3& v2) const;

public:
    AABB localAABB;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::string meshName;

private:
    std::string m_Path;
    bool m_Loaded = false;
};

}