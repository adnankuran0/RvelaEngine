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

json MeshRendererComponent::Serialize() const
{
    json j;
    j["castShadows"] = castShadow;
    return j;
}

void MeshRendererComponent::Deserialize(const json& j)
{
    if (j.contains("castShadows"))
        castShadow = j["castShadows"].get<bool>();
    else if (j.contains("castShadow"))
        castShadow = j["castShadow"].get<bool>();
}
