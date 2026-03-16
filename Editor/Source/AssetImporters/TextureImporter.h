#pragma once
#include "Asset/IAssetImporter.h"

namespace rv {

class TextureImporter : public IAssetImporter
{
public:
    std::string GetImporterID() const override { return "TextureImporter"; }

    std::vector<std::string> GetSupportedExtensions() const override
    {
        return { ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".hdr" };
    }

    std::filesystem::path GetCachePath(
        const std::filesystem::path& sourcePath,
        const AssetMeta& meta,
        const std::filesystem::path& cacheRoot) const override;

    bool Import(
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& outCachePath,
        const std::string& settingsJson) override;
};

}