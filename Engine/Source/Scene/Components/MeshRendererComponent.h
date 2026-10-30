#pragma once
#include "Renderer/GPUMesh.h"
#include <Renderer/AABB.h>
#include "Asset/Types/MeshAsset.h"
#include "Core/Ref.h"
#include <memory>

namespace rv {

struct alignas(16) MeshRendererComponent 
{
public:
    AABB localAABB;
    AABB worldAABB;
    unsigned int indexCount = 0;
    std::shared_ptr<GPUMesh> gpuMesh = nullptr;

    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = default;
    MeshRendererComponent& operator=(const MeshRendererComponent&) = default;
    MeshRendererComponent(MeshRendererComponent&&) = default;
    MeshRendererComponent& operator=(MeshRendererComponent&&) = default;

    MeshRendererComponent(Ref<MeshAsset> mesh)
    {
        if (mesh)
            RecreateFromMesh(mesh);
    }

    void RecreateFromMesh(Ref<MeshAsset> mesh);

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
