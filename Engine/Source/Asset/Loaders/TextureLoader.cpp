#include "rvelapch.h"
#include "TextureLoader.h"
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

    std::vector<uint8_t> pixels(header.dataSize);
    file.read(reinterpret_cast<char*>(pixels.data()), header.dataSize);

    auto asset = CreateRef<TextureAsset>(meta.uuid);
    asset->m_Width = header.width;
    asset->m_Height = header.height;
    asset->m_MipCount = header.mipCount;
    asset->m_Format = header.format;
    asset->m_SRGB = header.isSRGB != 0;
    asset->m_Pixels = std::move(pixels);

    return asset;
}

