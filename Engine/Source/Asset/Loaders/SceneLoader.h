#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class SceneLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "SceneLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rscene" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
};

}