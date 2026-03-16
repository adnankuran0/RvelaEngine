#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class MeshLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "MeshLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rmesh" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
};

}