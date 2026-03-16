#pragma once
#include "Asset/Types/TextureAsset.h"

namespace rv {

constexpr uint32_t MAGIC_TEXTURE = 0x52544558; // "RTEX"
constexpr uint16_t TEXTURE_CACHE_VERSION = 1;

#pragma pack(push, 1)
    struct TextureCacheHeader
    {
        uint32_t magic = MAGIC_TEXTURE;
        uint16_t version = TEXTURE_CACHE_VERSION;
        uint32_t width = 0;
        uint32_t height = 0;
        uint16_t mipCount = 1;
        TextureFormat format = TextureFormat::Unknown;
        uint8_t isSRGB = 0;
        uint8_t reserved[5] = {}; // alignment
    };
#pragma pack(pop)

}