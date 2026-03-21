#include "rvelapch.h"
#include "TextureImporter.h"
#include "Asset/CacheTypes/TextureCacheHeader.h"
#include "Asset/AssetMeta.h"
#include "Core/Log.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "stb_image/stb_image.h"
#include <algorithm>
#include <cctype>

using namespace rv;
using json = nlohmann::json;

struct TextureImportSettings
{
    bool sRGB = true;
    bool generateMips = false;
    int maxSize = 2048;
};

static TextureImportSettings ParseSettings(const std::string& settingsJson)
{
    TextureImportSettings settings;
    if (settingsJson.empty() || settingsJson == "{}")
        return settings;

    auto j = json::parse(settingsJson, nullptr, false);
    if (j.is_discarded())
        return settings;

    settings.sRGB = j.value("sRGB", false);
    settings.generateMips = j.value("generateMips", false);
    settings.maxSize = j.value("maxSize", 2048);
    return settings;
}

std::filesystem::path TextureImporter::GetCachePath(
    const std::filesystem::path& sourcePath,
    const AssetMeta& meta,
    const std::filesystem::path& cacheRoot) const
{
    return cacheRoot / (meta.uuid.ToString() + ".rtex");
}

bool TextureImporter::Import(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& outCachePath,
    const std::string& settingsJson)
{
    auto settings = ParseSettings(settingsJson);

    std::string filename = sourcePath.filename().string();
    std::string lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

    int desiredChannels = 0;         
    bool forceSRGB = settings.sRGB;
    bool isAlbedo = false;
    bool isNormal = false;
    bool isORM = false;

    if (lowerFilename.find("ao") != std::string::npos ||
        lowerFilename.find("rough") != std::string::npos ||
        lowerFilename.find("metal") != std::string::npos)
    {
        desiredChannels = 1;
        forceSRGB = false;
        isORM = true;
    }
    else if (lowerFilename.find("nor") != std::string::npos ||
        lowerFilename.find("normal") != std::string::npos)
    {
        desiredChannels = 3;
        forceSRGB = false;
        isNormal = true;
    }
    else if (lowerFilename.find("diff") != std::string::npos ||
        lowerFilename.find("albedo") != std::string::npos ||
        lowerFilename.find("basecolor") != std::string::npos)
    {
        desiredChannels = 4;
        forceSRGB = true;
        isAlbedo = true;
    }
    else
    {
        desiredChannels = 0;
    }

    stbi_set_flip_vertically_on_load(true);
    int w, h, channels;
    uint8_t* pixels = stbi_load(sourcePath.string().c_str(), &w, &h, &channels, desiredChannels);
    if (!pixels)
    {
        LOG_ERROR("stb_image failed: {}", sourcePath.string());
        return false;
    }

    if (w > settings.maxSize || h > settings.maxSize)
        LOG_WARN("Texture exceeds maxSize ({}): {}x{} — {}",
            settings.maxSize, w, h, sourcePath.string());

    int loadedChannels = desiredChannels > 0 ? desiredChannels : channels;

    assert(!(isNormal && loadedChannels != 3) && "Normal map must be RGB8!");

    TextureFormat format = TextureFormat::Unknown;
    switch (loadedChannels)
    {
    case 1: format = TextureFormat::R8; break;
    case 3: format = TextureFormat::RGB8; break;
    case 4: format = TextureFormat::RGBA8; break;
    default:
        LOG_ERROR("Unsupported channel count: {}", loadedChannels);
        stbi_image_free(pixels);
        return false;
    }

    TextureCacheHeader header{};
    header.width = static_cast<uint32_t>(w);
    header.height = static_cast<uint32_t>(h);
    header.format = format;
    header.isSRGB = forceSRGB ? 1 : 0;

    size_t pixelSize = static_cast<size_t>(w) * h * loadedChannels;

   
    std::ofstream file(outCachePath, std::ios::binary);
    if (!file)
    {
        stbi_image_free(pixels);
        LOG_ERROR("Cannot write cache: {}", outCachePath.string());
        return false;
    }

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(pixels), pixelSize);

    stbi_image_free(pixels);
    return true;
}