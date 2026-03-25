#pragma once
#include "Asset/IAssetImporter.h"
#include "DirectXTex/DirectXTex.h"
#include "Asset/Types/TextureAsset.h"

namespace rv {

struct TextureImportSettings
{
    bool sRGB = false;
    bool generateMips = false;
    int maxSize = 4096;
    bool compress = true;
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
    DXGI_FORMAT ToDXGI(TextureFormat format, bool sRGB);
    unsigned int CalculateMipCount(unsigned int w, unsigned int h);
    std::vector<Mip> GenerateMipChain(unsigned char* data, int w, int h, int channels, 
        unsigned int mipCount, bool isSRGB = false, bool isNormal = false);
    std::vector<DirectX::Image> BuildImages(const std::vector<TextureImporter::Mip>& mips, int channels,
        std::vector<std::vector<uint8_t>>& expandedBuffers);
    std::vector<uint8_t> CompressMips(
        const std::vector<TextureImporter::Mip>& mips,
        int channels,
        TextureFormat format,
        bool isSRGB);
};

}