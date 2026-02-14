#include "rvelapch.h"
#include "MeshAsset.h"

#include "Core/Log.h"
#include <fstream>
#include <span>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

void PackVertices(std::span<const Vertex> verts, std::vector<float>& out) 
{ 
    out.clear();
    out.reserve(verts.size() * 14); 
    for (const auto& v : verts) 
    { out.insert(out.end(), glm::value_ptr(v.position), glm::value_ptr(v.position) + 3); 
    out.insert(out.end(), glm::value_ptr(v.normal), glm::value_ptr(v.normal) + 3); 
    out.insert(out.end(), glm::value_ptr(v.tangent), glm::value_ptr(v.tangent) + 3); 
    out.insert(out.end(), glm::value_ptr(v.bitangent), glm::value_ptr(v.bitangent) + 3); 
    out.insert(out.end(), glm::value_ptr(v.texCoord), glm::value_ptr(v.texCoord) + 2); 
    } 
}

std::unique_ptr<AssetMeta> MeshMeta::Clone() const
{
    return std::make_unique<MeshMeta>(*this);
}

void MeshMeta::Serialize(std::vector<char>& outBuffer, size_t& offset) const
{
    SerializeBase(outBuffer, offset);

    if (outBuffer.size() < offset + sizeof(vertexCount) + sizeof(indexCount))
        outBuffer.resize(offset + sizeof(vertexCount) + sizeof(indexCount));

    std::memcpy(outBuffer.data() + offset, &vertexCount, sizeof(vertexCount));
    offset += sizeof(vertexCount);

    std::memcpy(outBuffer.data() + offset, &indexCount, sizeof(indexCount));
    offset += sizeof(indexCount);
}

void MeshMeta::Deserialize(const std::vector<char>& inBuffer, size_t& offset)
{
    DeserializeBase(inBuffer, offset);

    std::memcpy(&vertexCount, inBuffer.data() + offset, sizeof(vertexCount));
    offset += sizeof(vertexCount);

    std::memcpy(&indexCount, inBuffer.data() + offset, sizeof(indexCount));
    offset += sizeof(indexCount);
}

MeshAsset::MeshAsset(const std::string& path, std::unique_ptr<MeshMeta> meshMeta)
    : Asset(std::move(meshMeta)), m_Path(path)
{
}

bool MeshAsset::IsLoaded() const
{
    return m_Loaded;
}

bool MeshAsset::Load()
{
    if (m_Loaded)
        return true;

    MeshMeta* meta = GetMetaAs<MeshMeta>();
    if (!meta)
    {
        LOG_ERROR("Failed to get MeshMeta");
        return false;
    }

    std::ifstream file(m_Path, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Failed to open mesh file: {}", m_Path);
        return false;
    }

    AssetHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (!file || header.magic != MAGIC_MESH || header.type != AssetType::Mesh)
    {
        LOG_ERROR("Invalid mesh header in: {}", m_Path);
        return false;
    }

    file.seekg(sizeof(AssetHeader) + header.metaSize, std::ios::beg);
    file.read(reinterpret_cast<char*>(&localAABB), sizeof(AABB));

    vertices.resize(meta->vertexCount);
    indices.resize(meta->indexCount);

    file.read(reinterpret_cast<char*>(vertices.data()),
        sizeof(Vertex) * vertices.size());

    file.read(reinterpret_cast<char*>(indices.data()),
        sizeof(uint32_t) * indices.size());

    if (vertices.empty() || indices.empty())
    {
        LOG_ERROR("Vertex or index data missing in mesh: {}", m_Path);
        return false;
    }

    m_Loaded = true;
    return true;
}

int MeshAsset::GetTriangleCount() const
{
    return static_cast<int>(indices.size()) / 3;
}

void MeshAsset::GetTriangle(int triangleIndex,
    glm::vec3& v0,
    glm::vec3& v1,
    glm::vec3& v2) const
{
    const unsigned int i0 = indices[triangleIndex * 3 + 0];
    const unsigned int i1 = indices[triangleIndex * 3 + 1];
    const unsigned int i2 = indices[triangleIndex * 3 + 2];

    v0 = vertices[i0].position;
    v1 = vertices[i1].position;
    v2 = vertices[i2].position;
}
