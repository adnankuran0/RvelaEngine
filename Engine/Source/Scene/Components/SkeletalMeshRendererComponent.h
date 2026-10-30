#pragma once
#include "Renderer/GPUMesh.h"
#include <Renderer/AABB.h>
#include "Asset/Types/SkeletalMeshAsset.h"
#include "Core/Ref.h"
#include <memory>

namespace rv {

struct alignas(16) SkeletalMeshRendererComponent
{
public:
    AABB localAABB;
    AABB worldAABB;
    unsigned int indexCount = 0;
    std::shared_ptr<GPUMesh> gpuMesh = nullptr;

    SkeletalMeshRendererComponent() = default;
    SkeletalMeshRendererComponent(const SkeletalMeshRendererComponent&) = default;
    SkeletalMeshRendererComponent& operator=(const SkeletalMeshRendererComponent&) = default;
    SkeletalMeshRendererComponent(SkeletalMeshRendererComponent&&) = default;
    SkeletalMeshRendererComponent& operator=(SkeletalMeshRendererComponent&&) = default;

    SkeletalMeshRendererComponent(Ref<SkeletalMeshAsset> mesh)
    {
        if (mesh)
            RecreateFromMesh(mesh);
    }

    void RecreateFromMesh(Ref<SkeletalMeshAsset> mesh);

    inline void Destroy() noexcept
    {
        gpuMesh = nullptr;
        indexCount = 0;
    }

    inline void Bind() const noexcept
    {
        if (gpuMesh && gpuMesh->VAO.getID() != 0)
            gpuMesh->Bind();
    }

    inline bool IsCastShadow() const noexcept { return castShadow; }
    inline void SetCastShadow(bool isCastsShadow) noexcept { castShadow = isCastsShadow; }

private:
    bool castShadow = true;
};

}
