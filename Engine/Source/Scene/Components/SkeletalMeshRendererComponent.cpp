#include "rvelapch.h"
#include "SkeletalMeshRendererComponent.h"
#include "Renderer/MeshCache.h"

using namespace rv;

void SkeletalMeshRendererComponent::RecreateFromMesh(Ref<SkeletalMeshAsset> mesh)
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

json SkeletalMeshRendererComponent::Serialize() const
{
    json j;
    j["castShadows"] = castShadow;
    return j;
}

void SkeletalMeshRendererComponent::Deserialize(const json& j)
{
    if (j.contains("castShadows"))
        castShadow = j["castShadows"].get<bool>();
    else if (j.contains("castShadow"))
        castShadow = j["castShadow"].get<bool>();
}
