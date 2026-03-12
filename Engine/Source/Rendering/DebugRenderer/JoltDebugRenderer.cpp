#include "rvelapch.h"
#include "JoltDebugRenderer.h"
#include "DebugRenderer.h"
#include "Math/RvelaMath.h"

using namespace rv;

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
	auto& renderer = rv::DebugRenderer::Get();
	renderer.DrawLine(math::FromJoltRVec3(inFrom), math::FromJoltRVec3(inTo), math::FromJoltColor(inColor));

}
