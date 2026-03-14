#include "rvelapch.h"
#include "BroadPhaseLayer.h"

using namespace rv::Physics;

BPLayerInterfaceImpl::BPLayerInterfaceImpl()
{
	mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
	mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
}

unsigned int BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
{
	return BroadPhaseLayers::NUM_LAYERS;
}

JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
{
	JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
	return mObjectToBroadPhase[inLayer];
}

