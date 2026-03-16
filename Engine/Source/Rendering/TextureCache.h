#pragma once
#include "Texture.h"
#include "Core/Ref.h"
#include "Asset/Types/TextureAsset.h"
#include "Asset/AssetUUID.h"
#include "tsl/robin_map.h"
#include <list>
#include <unordered_set>

namespace rv {

class TextureCache
{
public:
    static TextureCache& Get()
    {
        static TextureCache instance;
        return instance;
    }

    void SetCapacity(size_t capacity) { m_Capacity = capacity; }

    Texture& GetOrCreate(const Ref<TextureAsset>& asset);

    Texture& GetFallback()
    {
        static Texture fallback = []() {
            uint8_t pixels[4] = { 255, 0, 255, 255 };
            Texture t;
            t.GenerateFromMemory(pixels, 1, 1, TextureFormat::RGBA8, false);
            return t;
            }();
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
    TextureCache() = default;

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

    tsl::robin_map<AssetUUID, Texture> m_Cache;
    std::list<AssetUUID> m_LRUList;     
    tsl::robin_map<AssetUUID, std::list<AssetUUID>::iterator> m_LRUPosition; 
};

}