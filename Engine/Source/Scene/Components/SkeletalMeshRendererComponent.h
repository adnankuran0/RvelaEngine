#pragma once
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/ElementBuffer.h"
#include <Renderer/AABB.h>
#include "Asset/Types/SkeletalMeshAsset.h"
#include "Core/Ref.h"

namespace rv {

struct alignas(16) SkeletalMeshRendererComponent
{
public:
    BufferLayout layout;
    AABB localAABB;
    AABB worldAABB;
    VertexArray VAO;
    VertexBuffer VBO{};
    ElementBuffer EBO{};
    unsigned int indexCount = 0;

    SkeletalMeshRendererComponent() = default;
    SkeletalMeshRendererComponent(const SkeletalMeshRendererComponent&) = delete;
    SkeletalMeshRendererComponent& operator=(const SkeletalMeshRendererComponent&) = delete;
    SkeletalMeshRendererComponent(SkeletalMeshRendererComponent&&) = default;
    SkeletalMeshRendererComponent& operator=(SkeletalMeshRendererComponent&&) = default;
    SkeletalMeshRendererComponent(Ref<SkeletalMeshAsset> mesh)
    {
        RecreateFromMesh(mesh);
    }

    void RecreateFromMesh(Ref<SkeletalMeshAsset> mesh);

    inline void Destroy() noexcept
    {
        VAO.Destroy();
        VBO.Destroy();
        EBO.Destroy();
    }

    inline bool IsCastShadow() const noexcept { return castShadow; }
    inline void SetCastShadow(bool isCastsShadow) noexcept { castShadow = isCastsShadow; }
private:
    bool castShadow = true;
};

}