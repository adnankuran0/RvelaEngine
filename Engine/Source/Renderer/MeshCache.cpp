#include "rvelapch.h"
#include "MeshCache.h"
#include "Core/Log.h"

using namespace rv;

std::shared_ptr<GPUMesh> MeshCache::GetOrCreate(const Ref<MeshAsset>& asset)
{
    if (!asset || !asset->IsValid())
    {
        return GetFallback();
    }

    const AssetUUID uuid = asset->GetUUID();

    auto it = m_Cache.find(uuid);
    if (it != m_Cache.end())
    {
        MoveToFront(uuid);
        return it.value();
    }

    if (m_Cache.size() >= m_Capacity)
        EvictLRU();

    auto gpuMesh = std::make_shared<GPUMesh>();

    std::vector<float> packedVertices;
    PackVertices(asset->GetVertices(), packedVertices);

    gpuMesh->indexCount = asset->GetIndexCount(0);
    gpuMesh->localAABB = asset->GetAABB();

    gpuMesh->lods.clear();
    if (asset->GetLODCount() > 0)
    {
        gpuMesh->lods.reserve(asset->GetLODCount());
        for (const auto& l : asset->GetLODs())
        {
            GPULOD glod;
            glod.indexOffset = l.indexOffset;
            glod.indexCount = l.indexCount;
            glod.screenRatio = l.screenRatio;
            glod.lodError = l.lodError;
            gpuMesh->lods.push_back(glod);
        }
    }
    else
    {
        GPULOD glod;
        glod.indexOffset = 0;
        glod.indexCount = gpuMesh->indexCount;
        glod.screenRatio = 0.5f;
        glod.lodError = 0.0f;
        gpuMesh->lods.push_back(glod);
    }

    gpuMesh->VAO.Init();
    gpuMesh->VAO.Bind();

    gpuMesh->VBO.Init(packedVertices.data(), packedVertices.size() * sizeof(float));
    gpuMesh->VBO.Bind();

    gpuMesh->layout = BufferLayout();
    gpuMesh->layout.BindVertexBuffer(gpuMesh->VBO.getID());
    gpuMesh->layout.Push<float>(3); // Position
    gpuMesh->layout.Push<float>(3); // Normal
    gpuMesh->layout.Push<float>(3); // Tangent
    gpuMesh->layout.Push<float>(2); // UV

    gpuMesh->VAO.SetBufferLayout(gpuMesh->layout);

    gpuMesh->EBO.Init(asset->GetAllIndices().data(), asset->GetAllIndices().size() * sizeof(unsigned int));
    gpuMesh->EBO.Bind();

    gpuMesh->VAO.Unbind();

    m_LRUList.push_front(uuid);
    m_LRUPosition[uuid] = m_LRUList.begin();

    m_Cache.emplace(uuid, gpuMesh);

    return gpuMesh;
}

std::shared_ptr<GPUMesh> MeshCache::GetOrCreate(const Ref<SkeletalMeshAsset>& asset)
{
    if (!asset || !asset->IsValid())
    {
        return GetFallback();
    }

    const AssetUUID uuid = asset->GetUUID();

    auto it = m_Cache.find(uuid);
    if (it != m_Cache.end())
    {
        MoveToFront(uuid);
        return it.value();
    }

    if (m_Cache.size() >= m_Capacity)
        EvictLRU();

    auto gpuMesh = std::make_shared<GPUMesh>();

    auto vertices = asset->GetVertices();
    gpuMesh->indexCount = (unsigned int)asset->GetIndices().size();
    gpuMesh->localAABB = asset->GetAABB();

    gpuMesh->lods.clear();
    GPULOD glod;
    glod.indexOffset = 0;
    glod.indexCount = gpuMesh->indexCount;
    glod.screenRatio = 0.5f;
    glod.lodError = 0.0f;
    gpuMesh->lods.push_back(glod);

    gpuMesh->VAO.Init();
    gpuMesh->VAO.Bind();

    gpuMesh->VBO.Init(vertices.data(), vertices.size() * sizeof(SkeletalVertex));
    gpuMesh->VBO.Bind();

    gpuMesh->layout = BufferLayout();
    gpuMesh->layout.BindVertexBuffer(gpuMesh->VBO.getID());
    gpuMesh->layout.Push<float>(3);   // Position
    gpuMesh->layout.Push<float>(3);   // Normal
    gpuMesh->layout.Push<float>(3);   // Tangent
    gpuMesh->layout.Push<float>(2);   // UV
    gpuMesh->layout.Push<uint32_t>(4);// Bone IDs
    gpuMesh->layout.Push<float>(4);   // Bone Weights

    gpuMesh->VAO.SetBufferLayout(gpuMesh->layout);

    gpuMesh->EBO.Init(asset->GetIndices().data(), asset->GetIndices().size() * sizeof(unsigned int));
    gpuMesh->EBO.Bind();

    gpuMesh->VAO.Unbind();

    m_LRUList.push_front(uuid);
    m_LRUPosition[uuid] = m_LRUList.begin();

    m_Cache.emplace(uuid, gpuMesh);

    return gpuMesh;
}

void MeshCache::Remove(const AssetUUID& uuid)
{
    auto it = m_Cache.find(uuid);
    if (it == m_Cache.end()) return;

    auto posIt = m_LRUPosition.find(uuid);
    if (posIt != m_LRUPosition.end())
    {
        m_LRUList.erase(posIt->second);
        m_LRUPosition.erase(posIt);
    }

    m_Cache.erase(it);
}
