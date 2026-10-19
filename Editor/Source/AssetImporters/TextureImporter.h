#pragma once
#include "Asset/IAssetImporter.h"
#include "DirectXTex/DirectXTex.h"
#include "Asset/Types/TextureAsset.h"

namespace rv {

enum class TextureImportType : int
{
    Auto = 0,
    Color,
    NormalMap,
    GrayscaleMask,
    HDR
};

enum class TextureCompressionFormat : int
{
    Auto = 0,
    BC1,
    BC3,
    BC4,
    BC5,
    Uncompressed
};

enum class TextureMipFilter : int
{
    Linear = 0,
    Box
};

struct TextureImportSettings
{
    TextureImportType textureType = TextureImportType::Auto;
    bool sRGB = true;
    bool generateMips = true;
    TextureMipFilter mipFilter = TextureMipFilter::Linear;
    int maxSize = 4096;
    bool compress = true;
    TextureCompressionFormat compressionFormat = TextureCompressionFormat::Auto;
    bool flipY = true;
    bool invertGreenChannel = false;
    bool normalizeNormals = true;
};

class TextureImporter : public IAssetImporter
{
public:
    std::string GetImporterID() const override { return "TextureImporter"; }
    uint32_t GetImporterVersion(const std::filesystem::path& sourcePath) const override;

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
        std::vector<std::vector<uint8_t>>& expandedBuffers, bool isSRGB = false);
    std::vector<uint8_t> CompressMips(
        const std::vector<TextureImporter::Mip>& mips,
        int channels,
        TextureFormat format,
        bool isSRGB);
};

}
