#pragma once
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/ElementBuffer.h"
#include <Scene/BoundingBox.h>

class alignas(16) MeshRendererComponent {
public:
    BufferLayout layout;
    BoundingBox localAABB;
    BoundingBox worldAABB;
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    unsigned int indexCount = 0;

    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = delete;
    MeshRendererComponent& operator=(const MeshRendererComponent&) = delete;
    MeshRendererComponent(MeshRendererComponent&&) = default;
    MeshRendererComponent& operator=(MeshRendererComponent&&) = default;
    MeshRendererComponent(void* vertices, size_t sizeOfVertices, void* indices, size_t sizeOfIndices, unsigned int indexCount, BoundingBox localAABB)
    {
        this->indexCount = indexCount;

        VAO.Bind();
        VBO.Init(vertices, sizeOfVertices);
        VBO.Bind();
        layout.BindVertexBuffer(VBO.getID());
        layout.Push<float>(3);
        layout.Push<float>(3);
        layout.Push<float>(2);
        VAO.SetBufferLayout(layout);
        EBO.Init(indices, sizeOfIndices);
        EBO.Bind();
        this->localAABB = localAABB;
    }

    void Destroy()
    {
        VAO.Destroy();
        VBO.Destroy();
        EBO.Destroy();
    }
};