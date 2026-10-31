#pragma once
#include "Asset/IAssetLoader.h"
#include "Asset/Types/TextureAsset.h"

namespace rv {

class TextureLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "TextureLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rtex" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
    Ref<Asset> LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta) override;
};

}
