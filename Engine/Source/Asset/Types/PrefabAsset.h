#pragma once
#include "Asset/Asset.h"

namespace rv {

class PrefabAsset : public Asset
{
public:
    explicit PrefabAsset(AssetUUID uuid) : Asset(uuid) {}

    const std::string& GetJSON() const { return m_JSON; }
    bool IsValid() const { return !m_JSON.empty(); }

private:
    friend class PrefabLoader;
    friend class PrefabImporter;
    std::string m_JSON;
};

}