#include "rvelapch.h"
#include "SkeletalMeshRendererComponent.h"

using namespace rv;

void SkeletalMeshRendererComponent::RecreateFromMesh(Ref<SkeletalMeshAsset> mesh)
{
    Destroy();
    if (!mesh || !mesh->IsValid()) return;

    indexCount = (unsigned int)mesh->GetIndices().size();

    VAO.Init();
    VAO.Bind();

    auto vertices = mesh->GetVertices();
    VBO.Init(vertices.data(), vertices.size() * sizeof(SkeletalVertex));
    VBO.Bind();

    layout = BufferLayout();
    layout.BindVertexBuffer(VBO.getID());
    layout.Push<float>(3);   //Position
    layout.Push<float>(3);   //Normal
    layout.Push<float>(3);   //Tangent
    layout.Push<float>(2);   //UV
    layout.Push<uint32_t>(4);//Bone IDs
    layout.Push<float>(4);   //Bone Weights

    VAO.SetBufferLayout(layout);

    EBO.Init(mesh->GetIndices().data(), mesh->GetIndices().size() * sizeof(unsigned int));
    EBO.Bind();

    localAABB = mesh->GetAABB();
}