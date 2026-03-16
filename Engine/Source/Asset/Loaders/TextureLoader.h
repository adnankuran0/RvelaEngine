#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class TextureLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "TextureLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rtex" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
};

}