#pragma once
#include "Asset/IAssetImporter.h"

namespace rv {

struct TextureImportSettings
{
    bool sRGB = false;
    bool generateMips = false;
    int maxSize = 2048;
};

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

    std::string GetDefaultSettings() const override;

    static TextureImportSettings ParseSettings(const std::string& settingsJson);
private:
    struct Mip
    {
        unsigned int mipLevel = 0;
        std::vector<unsigned char> data;
        int w;
        int h;
    };
private:
    unsigned int CalculateMipCount(unsigned int w, unsigned int h);
    std::vector<Mip> GenerateMipChain(unsigned char* data, int w, int h, int channels, 
        unsigned int mipCount, bool isSRGB = false, bool isNormal = false);

};

}