#pragma once
#include "Jolt/Jolt.h"
#include "Jolt/Renderer/DebugRenderer.h"

namespace rv {

class TriangleBatchImpl : public JPH::RefTarget<TriangleBatchImpl>, public JPH::RefTargetVirtual
{
public:
	std::vector<glm::vec3> vertices;
	std::vector<uint32_t> indices;

	virtual void			AddRef() override { JPH::RefTarget<TriangleBatchImpl>::AddRef(); }
	virtual void			Release() override { JPH::RefTarget<TriangleBatchImpl>::Release(); }
};


class JoltDebugRenderer : public JPH::DebugRenderer
{
public:
	JoltDebugRenderer() { Initialize(); }
	virtual void						DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
	virtual void						DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override;
	virtual Batch						CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;
	virtual Batch						CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) override;
	virtual void						DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode) override;
	virtual void						DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {}

};

}
