#include "rvelapch.h"
#include "MeshRendererComponent.h"

void MeshRendererComponent::RecreateFromMesh(Ref<MeshAsset> mesh)
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

    //worldAABB = localAABB;
}