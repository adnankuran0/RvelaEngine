#pragma once
#include "Core/Ref.h"
#include "AssetUUID.h"
#include "AssetMeta.h"

class Asset : public RefCounted 
{
public:
    Asset(std::unique_ptr<AssetMeta> assetMeta) : m_Meta(std::move(assetMeta)) {}
    virtual ~Asset() = default;

    inline AssetUUID GetUUID() const { return m_Meta.get()->uuid; }
    inline void SetUUID(const AssetUUID& uuid) { m_Meta.get()->uuid = uuid; }
    inline AssetType GetAssetType() const { return m_Meta.get()->type; }
    //inline bool IsAlive() const { return !IsDestroyed(); }

    template<typename T>
    inline T* GetMetaAs() 
    {
        return dynamic_cast<T*>(m_Meta.get());
    }


protected:
    std::unique_ptr<AssetMeta> m_Meta;
};