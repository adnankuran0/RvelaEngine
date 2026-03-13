#include "rvelapch.h"
#include "JoltDebugRenderer.h"
#include "DebugRenderer.h"
#include "Math/RvelaMath.h"

using namespace rv;

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
	auto& renderer = rv::DebugRenderer::Get();
	renderer.DrawLine(math::FromJoltRVec3(inFrom), math::FromJoltRVec3(inTo), m_JoltLineColor);

}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow)
{
	auto& renderer = rv::DebugRenderer::Get();
	renderer.DrawTriangle(math::FromJoltRVec3(inV1), math::FromJoltRVec3(inV2), math::FromJoltRVec3(inV3), m_JoltLineColor);
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount)
{
    auto* batch = new TriangleBatchImpl();

    batch->vertices.reserve(inTriangleCount * 3);
    batch->indices.reserve(inTriangleCount * 3);

    for (int i = 0; i < inTriangleCount; i++)
    {
        const auto& t = inTriangles[i];

        uint32_t base = (uint32_t)batch->vertices.size();

        batch->vertices.push_back(math::FromJoltFloat3(t.mV[0].mPosition));
        batch->vertices.push_back(math::FromJoltFloat3(t.mV[1].mPosition));
        batch->vertices.push_back(math::FromJoltFloat3(t.mV[2].mPosition));

        batch->indices.push_back(base + 0);
        batch->indices.push_back(base + 1);
        batch->indices.push_back(base + 2);
    }

    return batch;
}


JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(
    const Vertex* inVertices,
    int inVertexCount,
    const JPH::uint32* inIndices,
    int inIndexCount)
{
    auto* batch = new TriangleBatchImpl();

    batch->vertices.reserve(inVertexCount);

    for (int i = 0; i < inVertexCount; i++)
        batch->vertices.push_back(math::FromJoltFloat3(inVertices[i].mPosition));

    batch->indices.reserve(inIndexCount);

    for (int i = 0; i < inIndexCount; i++)
        batch->indices.push_back(inIndices[i]);

    return batch;
}

void JoltDebugRenderer::DrawGeometry(
    JPH::RMat44Arg inModelMatrix,
    const JPH::AABox& inWorldSpaceBounds,
    float inLODScaleSq,
    JPH::ColorArg inModelColor,
    const GeometryRef& inGeometry,
    ECullMode inCullMode,
    ECastShadow inCastShadow,
    EDrawMode inDrawMode)
{
    if (!inGeometry || inGeometry->mLODs.empty()) return;

    const auto& batch = inGeometry->mLODs[0].mTriangleBatch;
    auto* batchImpl = static_cast<TriangleBatchImpl*>(batch.GetPtr());
    if (!batchImpl) return;

    auto& renderer = rv::DebugRenderer::Get();
    glm::vec4 color = math::FromJoltColor(inModelColor);

    const auto& verts = batchImpl->vertices;
    const auto& indices = batchImpl->indices;

    glm::mat4 model = math::FromJoltMat44(inModelMatrix);

    for (size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        glm::vec3 v0 = glm::vec3(model * glm::vec4(verts[indices[i + 0]], 1.0f));
        glm::vec3 v1 = glm::vec3(model * glm::vec4(verts[indices[i + 1]], 1.0f));
        glm::vec3 v2 = glm::vec3(model * glm::vec4(verts[indices[i + 2]], 1.0f));
        renderer.DrawTriangle(v0, v1, v2, m_JoltLineColor);
    }
}
