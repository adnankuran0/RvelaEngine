#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class SkeletalMeshLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "SkeletalMeshLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rskmesh" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
};

}