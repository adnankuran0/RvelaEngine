#include "rvelapch.h"
#include "SkeletalMeshAsset.h"
#include <glm/gtc/type_ptr.hpp>

using namespace rv;

void rv::PackVertices(std::span<const SkeletalVertex> verts, std::vector<float>& out)
{
    out.clear();
    out.reserve(verts.size() * 19);

    for (const auto& v : verts)
    {
        out.insert(out.end(), glm::value_ptr(v.position), glm::value_ptr(v.position) + 3);
        out.insert(out.end(), glm::value_ptr(v.normal), glm::value_ptr(v.normal) + 3);
        out.insert(out.end(), glm::value_ptr(v.tangent), glm::value_ptr(v.tangent) + 3);

        out.insert(out.end(), glm::value_ptr(v.texCoord), glm::value_ptr(v.texCoord) + 2);

        for (int i = 0; i < 4; i++)
        {
            out.push_back(static_cast<float>(v.boneIDs[i]));
        }

        for (int i = 0; i < 4; i++)
        {
            out.push_back(v.weights[i]);
        }
    }
}