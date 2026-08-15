#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {
    class AudioClipLoader : public IAssetLoader
    {
    public:
        std::string GetLoaderID() const override { return "AudioClipLoader"; }
        std::vector<std::string> GetSupportedExtensions() const override { return { ".wav", ".mp3", ".flac" }; }
        Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
    };
}