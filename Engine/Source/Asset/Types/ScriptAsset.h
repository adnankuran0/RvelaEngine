#pragma once
#include "Asset/Asset.h"

namespace rv {

class ScriptAsset : public Asset
{
public:
    explicit ScriptAsset(AssetUUID uuid) : Asset(uuid) {}

    const std::string& GetSource() const { return m_Source; }
    const std::string& GetScriptName() const { return m_ScriptName; }
    bool IsValid() const { return !m_Source.empty(); }

private:
    friend class ScriptLoader;
    std::string m_Source;
    std::string m_ScriptName;
};

}