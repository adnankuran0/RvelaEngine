#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class SkeletonLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "SkeletonLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rskeleton" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
};

}