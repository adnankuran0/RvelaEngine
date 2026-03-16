#pragma once
#include "Asset/Asset.h"
#include <filesystem>

namespace rv {

class SceneAsset : public Asset
{
public:
    explicit SceneAsset(AssetUUID uuid) : Asset(uuid) {}

    const std::string& GetJSON() const { return m_JSON; }

    bool IsValid() const { return !m_JSON.empty(); }

private:
    friend class SceneLoader;
    std::string m_JSON;
};

}