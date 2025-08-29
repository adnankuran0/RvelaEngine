#pragma once
#include "Renderer/VertexArray.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/ElementBuffer.h"
#include <Scene/AABB.h>
#include "Assets/MeshAsset.h"
#include "Core/Ref.h"

struct alignas(16) MeshRendererComponent {
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

    void RecreateFromMesh(Ref<MeshAsset> mesh)
    {
        Destroy();

        std::vector<float> packedVertices;
        PackVertices(mesh->vertices, packedVertices);

        indexCount = (unsigned int)mesh->indices.size();

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
        localAABB = mesh->localAABB;
        worldAABB = localAABB;
    }

    void Destroy()
    {
        VAO.Destroy();
        VBO.Destroy();
        EBO.Destroy();
    }

    inline bool IsCastShadow() const { return castShadow; }
    inline void SetCastShadow(bool isCastsShadow) { castShadow = isCastsShadow; }

    inline bool IsDoubleSided() const { return isDoubleSided; }
    inline void SetDoubleSided(bool isDoubleSided) { this->isDoubleSided = isDoubleSided; }
 private:
    bool castShadow = true;
    bool isDoubleSided = false;
};