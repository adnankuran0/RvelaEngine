#include "rvelapch.h"
#include "MeshAsset.h"
#include <glm/gtc/type_ptr.hpp>

using namespace rv;

void rv::PackVertices(std::span<const Vertex> verts, std::vector<float>& out)
{
    out.clear();
    out.reserve(verts.size() * 14); // maybe 11?
    for (const auto& v : verts)
    {
        out.insert(out.end(), glm::value_ptr(v.position), glm::value_ptr(v.position) + 3);
        out.insert(out.end(), glm::value_ptr(v.normal), glm::value_ptr(v.normal) + 3);
        out.insert(out.end(), glm::value_ptr(v.tangent), glm::value_ptr(v.tangent) + 3);
        out.insert(out.end(), glm::value_ptr(v.texCoord), glm::value_ptr(v.texCoord) + 2);
    }
}

void MeshAsset::GetTriangle(int triangleIndex,glm::vec3& v0, glm::vec3& v1, glm::vec3& v2) const
{
    const unsigned int i0 = m_Indices[triangleIndex * 3 + 0];
    const unsigned int i1 = m_Indices[triangleIndex * 3 + 1];
    const unsigned int i2 = m_Indices[triangleIndex * 3 + 2];
    v0 = m_Vertices[i0].position;
    v1 = m_Vertices[i1].position;
    v2 = m_Vertices[i2].position;
}