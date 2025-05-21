#pragma once
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/ElementBuffer.h"


class MeshRendererComponent {
public:
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    BufferLayout Layout;
    unsigned int indexCount = 0;
    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = delete;
    MeshRendererComponent& operator=(const MeshRendererComponent&) = delete;
    MeshRendererComponent(MeshRendererComponent&&) = default;
    MeshRendererComponent& operator=(MeshRendererComponent&&) = default;
    MeshRendererComponent(void* vertices, size_t sizeOfVertices, void* indices, size_t sizeOfIndices, unsigned int indexCount)
    {
        this->indexCount = indexCount;

        VAO.Bind();
        VBO.Init(vertices, sizeOfVertices);
        VBO.Bind();
        Layout.BindVertexBuffer(VBO.getID());
        Layout.Push<float>(3);
        Layout.Push<float>(3);
        Layout.Push<float>(2);
        VAO.SetBufferLayout(Layout);
        EBO.Init(indices, sizeOfIndices);
        EBO.Bind();
    }

    void Destroy()
    {
        VAO.Destroy();
        VBO.Destroy();
        EBO.Destroy();
    }
};