#pragma once
#include <memory>
#include "AssetMeta.h"

namespace rv {

class Asset 
{
public:
    Asset(std::unique_ptr<AssetMeta> assetMeta);
    virtual ~Asset() = default;

    AssetUUID GetUUID() const; 
    void SetUUID(const AssetUUID& uuid); 
    AssetType GetAssetType() const; 
    //inline bool IsAlive() const { return !IsDestroyed(); }

    template<typename T>
    inline T* GetMetaAs() 
    {
        return dynamic_cast<T*>(m_Meta.get());
    }


protected:
    std::unique_ptr<AssetMeta> m_Meta;
};

}