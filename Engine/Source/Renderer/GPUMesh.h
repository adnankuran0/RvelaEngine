#pragma once
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "ElementBuffer.h"
#include "BufferLayout.h"
#include "AABB.h"
#include <vector>

namespace rv {

struct GPULOD
{
    unsigned int indexOffset = 0;
    unsigned int indexCount = 0;
    float screenRatio = 0.0f;
    float lodError = 0.0f;
};

struct GPUMesh
{
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    BufferLayout layout;
    unsigned int indexCount = 0;
    AABB localAABB;
    std::vector<GPULOD> lods;

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
