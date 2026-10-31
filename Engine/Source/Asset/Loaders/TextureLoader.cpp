#include "rvelapch.h"
#include "TextureLoader.h"
#include "Asset/CacheTypes/TextureCacheHeader.h"
#include "Core/Log.h"
#include "Asset/AssetMeta.h"
#include <fstream>
#include <cstring>

namespace rv {

Ref<Asset> TextureLoader::LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta)
{
    if (!data || size < sizeof(TextureCacheHeader))
    {
        LOG_ERROR("TextureLoader::LoadFromMemory: Buffer too small");
        return nullptr;
    }

    TextureCacheHeader header{};
    std::memcpy(&header, data, sizeof(header));

    if (header.magic != MAGIC_TEXTURE)
    {
        LOG_ERROR("TextureLoader::LoadFromMemory: Invalid magic (got 0x{:X})", header.magic);
        return nullptr;
    }

    if (header.version != TEXTURE_CACHE_VERSION)
    {
        LOG_ERROR("TextureLoader::LoadFromMemory: Version mismatch (got {}, expected {})",
            header.version, TEXTURE_CACHE_VERSION);
        return nullptr;
    }

    if (header.width == 0 || header.height == 0)
    {
        LOG_ERROR("TextureLoader::LoadFromMemory: Invalid dimensions");
        return nullptr;
    }

    if (sizeof(header) + header.dataSize > size)
    {
        LOG_ERROR("TextureLoader::LoadFromMemory: Incomplete data in buffer");
        return nullptr;
    }

    std::vector<uint8_t> pixels(header.dataSize);
    std::memcpy(pixels.data(), data + sizeof(header), header.dataSize);

    auto asset = CreateRef<TextureAsset>(meta.uuid);
    asset->m_Width = header.width;
    asset->m_Height = header.height;
    asset->m_MipCount = header.mipCount;
    asset->m_Format = header.format;
    asset->m_SRGB = header.isSRGB != 0;
    asset->m_Pixels = std::move(pixels);

    return asset;
}

Ref<Asset> TextureLoader::Load(const std::filesystem::path& assetPath, const AssetMeta& meta)
{
    std::ifstream file(assetPath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        LOG_ERROR("Cannot open: {}", assetPath.string());
        return nullptr;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < static_cast<std::streamsize>(sizeof(TextureCacheHeader)))
    {
        LOG_ERROR("File too small: {}", assetPath.string());
        return nullptr;
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
    {
        LOG_ERROR("Failed to read file: {}", assetPath.string());
        return nullptr;
    }

    return LoadFromMemory(buffer.data(), buffer.size(), meta);
}

}
