#pragma once
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/ElementBuffer.h"
#include <Scene/BoundingBox.h>
#include "Assets/MeshAsset.h"
#include "Core/Ref.h"

class alignas(16) MeshRendererComponent {
public:
    BufferLayout layout;
    //BoundingBox localAABB;
    //BoundingBox worldAABB;
    VertexArray VAO;
    VertexBuffer VBO;
    ElementBuffer EBO;
    unsigned int indexCount = 0;

    MeshRendererComponent() = default;
    MeshRendererComponent(const MeshRendererComponent&) = delete;
    MeshRendererComponent& operator=(const MeshRendererComponent&) = delete;
    MeshRendererComponent(MeshRendererComponent&&) = default;
    MeshRendererComponent& operator=(MeshRendererComponent&&) = default;
    MeshRendererComponent(void* vertices, size_t sizeOfVertices, void* indices, size_t sizeOfIndices, unsigned int indexCount )
    {
        this->indexCount = indexCount;

        VAO.Init();
        VAO.Bind();
        VBO.Init(vertices, sizeOfVertices);
        VBO.Bind();
        layout.BindVertexBuffer(VBO.getID());
        layout.Push<float>(3); // Position
        layout.Push<float>(3); // Normal
        layout.Push<float>(3); // Tangent
        layout.Push<float>(3); // Bitangent
        layout.Push<float>(2); // UV
        VAO.SetBufferLayout(layout);
        EBO.Init(indices, sizeOfIndices);
        EBO.Bind();
        //this->localAABB = localAABB;
    }

    void RecreateFromMesh(Ref<MeshAsset> mesh)
    {
        Destroy();

        std::vector<float> packedVertices;
        PackVertices(mesh->vertices, packedVertices);

        indexCount = mesh->indices.size();

        VAO.Init();
        VAO.Bind();

        VBO.Init(packedVertices.data(), packedVertices.size() * sizeof(float));
        VBO.Bind();

        layout = BufferLayout(); // Reset layout before pushing again
        layout.BindVertexBuffer(VBO.getID());
        layout.Push<float>(3); // Position
        layout.Push<float>(3); // Normal
        layout.Push<float>(3); // Tangent
        layout.Push<float>(3); // Bitangent
        layout.Push<float>(2); // UV

        VAO.SetBufferLayout(layout);

        EBO.Init(mesh->indices.data(), mesh->indices.size() * sizeof(unsigned int));
        EBO.Bind();
    }

    void Destroy()
    {
        VAO.Destroy();
        VBO.Destroy();
        EBO.Destroy();
    }
};