#pragma once
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/ElementBuffer.h"
#include <Renderer/AABB.h>
#include "Asset/Types/MeshAsset.h"
#include "Core/Ref.h"

namespace rv {

struct alignas(16) MeshRendererComponent 
{
public:
    BufferLayout layout;
    AABB localAABB;
    AABB worldAABB;
    VertexArray VAO;
    VertexBuffer VBO{};
    ElementBuffer EBO{};
    unsigned int indexCount = 0;

    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = delete;
    MeshRendererComponent& operator=(const MeshRendererComponent&) = delete;
    MeshRendererComponent(MeshRendererComponent&&) = default;
    MeshRendererComponent& operator=(MeshRendererComponent&&) = default;
    MeshRendererComponent(Ref<MeshAsset> mesh)
    {
        RecreateFromMesh(mesh);
    }

    void RecreateFromMesh(Ref<MeshAsset> mesh);

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