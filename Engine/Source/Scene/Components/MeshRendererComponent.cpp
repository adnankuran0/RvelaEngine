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
        currentLOD = 0;
        return;
    }

    gpuMesh = MeshCache::Get().GetOrCreate(mesh);
    if (gpuMesh)
    {
        indexCount = !gpuMesh->lods.empty() ? gpuMesh->lods[0].indexCount : gpuMesh->indexCount;
        localAABB = gpuMesh->localAABB;
        currentLOD = 0;
    }
    else
    {
        indexCount = 0;
        localAABB.Reset();
        currentLOD = 0;
    }
}

json MeshRendererComponent::Serialize() const
{
    json j;
    j["castShadows"] = castShadow;
    j["enableAutoLOD"] = enableAutoLOD;
    j["lodBias"] = lodBias;
    return j;
}

void MeshRendererComponent::Deserialize(const json& j)
{
    if (j.contains("castShadows"))
        castShadow = j["castShadows"].get<bool>();
    else if (j.contains("castShadow"))
        castShadow = j["castShadow"].get<bool>();

    if (j.contains("enableAutoLOD"))
        enableAutoLOD = j["enableAutoLOD"].get<bool>();
    if (j.contains("lodBias"))
        lodBias = j["lodBias"].get<float>();
}
