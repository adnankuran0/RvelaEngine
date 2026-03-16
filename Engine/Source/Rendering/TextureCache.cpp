#include "rvelapch.h"
#include "TextureCache.h"

using namespace rv;

Texture& TextureCache::GetOrCreate(const Ref<TextureAsset>& asset)
{
    if (!asset)
    {
        LOG_WARN("Null texture asset, returning fallback");
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

    Texture tex;
    tex.GenerateFromMemory(
        asset->GetPixels().data(),
        asset->GetWidth(),
        asset->GetHeight(),
        asset->GetFormat(),
        asset->IsSRGB()
    );

    m_LRUList.push_front(uuid);
    m_LRUPosition[uuid] = m_LRUList.begin();

    m_Cache.emplace(uuid, std::move(tex));

    return m_Cache.find(uuid).value();
}

void TextureCache::Remove(const AssetUUID& uuid)
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