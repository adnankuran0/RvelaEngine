#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class ScriptLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "ScriptLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".lua" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
    Ref<Asset> LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta) override;
};

}
