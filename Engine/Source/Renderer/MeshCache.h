#pragma once
#include "GPUMesh.h"
#include "Core/Ref.h"
#include "Asset/AssetUUID.h"
#include "Asset/Types/MeshAsset.h"
#include "Asset/Types/SkeletalMeshAsset.h"
#include "tsl/robin_map.h"
#include <list>
#include <memory>

namespace rv {

class MeshCache
{
public:
    static MeshCache& Get()
    {
        static MeshCache instance;
        return instance;
    }

    void SetCapacity(size_t capacity) { m_Capacity = capacity; }

    std::shared_ptr<GPUMesh> GetOrCreate(const Ref<MeshAsset>& asset);
    std::shared_ptr<GPUMesh> GetOrCreate(const Ref<SkeletalMeshAsset>& asset);

    std::shared_ptr<GPUMesh> GetFallback()
    {
        static auto fallback = std::make_shared<GPUMesh>();
        return fallback;
    }

    void Remove(const AssetUUID& uuid);

    void Clear()
    {
        m_Cache.clear();
        m_LRUList.clear();
        m_LRUPosition.clear();
    }

    size_t GetSize() const { return m_Cache.size(); }
    size_t GetCapacity() const { return m_Capacity; }

private:
    MeshCache() = default;

    void MoveToFront(const AssetUUID& uuid)
    {
        auto posIt = m_LRUPosition.find(uuid);
        if (posIt == m_LRUPosition.end()) return;
        m_LRUList.splice(m_LRUList.begin(), m_LRUList, posIt->second);
        m_LRUPosition[uuid] = m_LRUList.begin();
    }

    void EvictLRU()
    {
        if (m_LRUList.empty()) return;

        AssetUUID lruUUID = m_LRUList.back();
        m_LRUList.pop_back();
        m_LRUPosition.erase(lruUUID);
        m_Cache.erase(lruUUID);
    }

    size_t m_Capacity = 512;

    tsl::robin_map<AssetUUID, std::shared_ptr<GPUMesh>> m_Cache;
    std::list<AssetUUID> m_LRUList;
    tsl::robin_map<AssetUUID, std::list<AssetUUID>::iterator> m_LRUPosition;
};

}
