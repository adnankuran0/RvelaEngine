#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

    class AnimationLibraryLoader : public IAssetLoader
    {
    public:
        std::string GetLoaderID() const override { return "AnimationLibraryLoader"; }
        std::vector<std::string> GetSupportedExtensions() const override { return { ".ranim", ".ranimlib" }; }
        Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
    };

}