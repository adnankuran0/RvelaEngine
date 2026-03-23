#include "rvelapch.h"
#include "TextureLoader.h"
#include "Asset/Types/TextureAsset.h"
#include "Asset/CacheTypes/TextureCacheHeader.h"
#include "Core/Log.h"
#include <fstream>
#include "Asset/AssetMeta.h"

using namespace rv;

Ref<Asset> TextureLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Cannot open: {}", assetPath.string());
        return nullptr;
    }

    TextureCacheHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!file)
    {
        LOG_ERROR("Failed to read header: {}", assetPath.string());
        return nullptr;
    }

    if (header.magic != MAGIC_TEXTURE)
    {
        LOG_ERROR("Invalid magic: {}", assetPath.string());
        return nullptr;
    }

    if (header.version != TEXTURE_CACHE_VERSION)
    {
        LOG_ERROR("Version mismatch (got {}, expected {}): {}",
            header.version, TEXTURE_CACHE_VERSION, assetPath.string());
        return nullptr;
    }

    if (header.width == 0 || header.height == 0)
    {
        LOG_ERROR("Invalid dimensions: {}", assetPath.string());
        return nullptr;
    }

    size_t bytesPerPixel = 0;
    switch (header.format)
    {
    case TextureFormat::R8:    bytesPerPixel = 1; break;
    case TextureFormat::RG8:   bytesPerPixel = 2; break;
    case TextureFormat::RGB8:  bytesPerPixel = 3; break;
    case TextureFormat::RGBA8: bytesPerPixel = 4; break;
    default:
        LOG_ERROR("Unsupported format: {}", assetPath.string());
        return nullptr;
    }

    size_t totalSize = 0;
    uint32_t mipW = header.width;
    uint32_t mipH = header.height;
    for (uint16_t i = 0; i < header.mipCount; i++)
    {
        totalSize += mipW * mipH * bytesPerPixel;
        mipW = std::max(1u, mipW / 2);
        mipH = std::max(1u, mipH / 2);
    }

    std::vector<uint8_t> pixels(totalSize);
    file.read(reinterpret_cast<char*>(pixels.data()), totalSize);

    auto asset = CreateRef<TextureAsset>(meta.uuid);
    asset->m_Width = header.width;
    asset->m_Height = header.height;
    asset->m_MipCount = header.mipCount;
    asset->m_Format = header.format;
    asset->m_SRGB = header.isSRGB != 0;
    asset->m_Pixels = std::move(pixels);

    return asset;
}