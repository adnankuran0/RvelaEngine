#pragma once
#include "Asset/Asset.h"
#include <span>

namespace rv {

enum class TextureFormat : uint8_t
{
    Unknown,
    R8,
    RG8,
    RGB8,
    RGBA8,
    BC1, // DXT1
    BC3, // DXT5
    BC4, // R compressed
    BC5, // RG compressed
};

class TextureAsset : public Asset
{
public:
    explicit TextureAsset(AssetUUID uuid) : Asset(uuid) {}

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }
    uint16_t GetMipCount() const { return m_MipCount; }
    TextureFormat GetFormat() const { return m_Format; }
    bool IsSRGB() const { return m_SRGB; }

    std::span<const uint8_t> GetPixels() const { return m_Pixels; }

    bool IsValid() const
    {
        return m_Width > 0 && m_Height > 0 && !m_Pixels.empty();
    }

private:
    friend class TextureLoader;

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint16_t m_MipCount = 1;
    TextureFormat m_Format = TextureFormat::Unknown;
    bool m_SRGB = false;
    std::vector<uint8_t> m_Pixels;
};

}