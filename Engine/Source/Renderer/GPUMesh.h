#pragma once
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "BufferLayout.h"
#include "AABB.h"

namespace rv {

struct GPUMesh
{
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    BufferLayout layout;
    unsigned int indexCount = 0;
    AABB localAABB;

    GPUMesh() = default;
    GPUMesh(const GPUMesh&) = delete;
    GPUMesh& operator=(const GPUMesh&) = delete;
    GPUMesh(GPUMesh&&) = default;
    GPUMesh& operator=(GPUMesh&&) = default;
    ~GPUMesh() = default;

    inline void Bind() const noexcept
    {
        VAO.Bind();
    }
};

}
