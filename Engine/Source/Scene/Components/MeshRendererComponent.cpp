#include "rvelapch.h"
#include "MeshRendererComponent.h"
#include "Renderer/MeshCache.h"

using namespace rv;

void MeshRendererComponent::RecreateFromMesh(Ref<MeshAsset> mesh)
{
    if (!mesh || !mesh->IsValid())
    {
        gpuMesh = nullptr;
        indexCount = 0;
        localAABB.Reset();
        return;
    }

    gpuMesh = MeshCache::Get().GetOrCreate(mesh);
    if (gpuMesh)
    {
        indexCount = gpuMesh->indexCount;
        localAABB = gpuMesh->localAABB;
    }
    else
    {
        indexCount = 0;
        localAABB.Reset();
    }
}
