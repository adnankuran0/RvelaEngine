#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class ShaderAsset;

class ShaderLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "ShaderLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rshader" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
};

}
