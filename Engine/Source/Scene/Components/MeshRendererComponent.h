#pragma once
#include "Rendering/VertexArray.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/ElementBuffer.h"
#include <Scene/AABB.h>
#include "Assets/MeshAsset.h"
#include "Core/Ref.h"

struct alignas(16) MeshRendererComponent 
{
public:
    BufferLayout layout;
    AABB localAABB;
    AABB worldAABB;
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
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

    inline bool IsDoubleSided() const noexcept { return isDoubleSided; }
    inline void SetDoubleSided(bool isDoubleSided) noexcept { this->isDoubleSided = isDoubleSided; }
 private:
    bool castShadow = true;
    bool isDoubleSided = false;
};