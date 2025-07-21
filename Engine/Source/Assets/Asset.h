#pragma once
#include "Core/Ref.h"
#include "AssetUUID.h"
#include "AssetMeta.h"



class Asset : public RefCounted {
public:
    Asset( AssetMeta assetMeta ) : m_Meta(assetMeta) {}
    virtual ~Asset() = default;
    inline AssetUUID GetUUID() { return  m_Meta.uuid; }
    inline void SetUUID(AssetUUID& uuid) { m_Meta.uuid = uuid; }
    inline AssetType GetAssetType() { return m_Meta.type; }

protected:
    AssetMeta m_Meta;
};
