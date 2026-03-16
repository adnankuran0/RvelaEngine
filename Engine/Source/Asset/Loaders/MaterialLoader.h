#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class MaterialLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "MaterialLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rmat" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
};

}