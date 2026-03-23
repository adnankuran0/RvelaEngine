#include "rvelapch.h"
#include "TextureImporter.h"
#include "Asset/CacheTypes/TextureCacheHeader.h"
#include "Asset/AssetMeta.h"
#include "Core/Log.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include "stb/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize2.h"
#include <algorithm>
#include <cctype>

using namespace rv;
using json = nlohmann::json;


TextureImportSettings TextureImporter::ParseSettings(const std::string& settingsJson)
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

unsigned int TextureImporter::CalculateMipCount(unsigned int w, unsigned int h)
{
    int size = std::max(w, h);
    unsigned int mipCount = 1;
    
    while (size > 1)
    {
        size /= 2;
        mipCount++;
    }

    return mipCount;
}

std::vector<TextureImporter::Mip> TextureImporter::GenerateMipChain(
    unsigned char* data,
    int w, int h,
    int channels,
    unsigned int mipCount,
    bool isSRGB,
    bool isNormal)
{
    std::vector<Mip> mips;

    // base level
    Mip base;
    base.w = w;
    base.h = h;
    base.data.assign(data, data + (w * h * channels));
    mips.push_back(base);

    stbir_pixel_layout layout;
    switch (channels)
    {
    case 1: layout = STBIR_1CHANNEL; break;
    case 2: layout = STBIR_2CHANNEL; break;
    case 3: layout = STBIR_RGB; break;
    case 4: layout = STBIR_RGBA; break;
    default: assert(false);
    }

    int currentW = w;
    int currentH = h;

    for (unsigned int i = 1; i < mipCount; i++)
    {
        int nextW = std::max(1, currentW / 2);
        int nextH = std::max(1, currentH / 2);

        Mip mip;
        mip.w = nextW;
        mip.h = nextH;
        mip.mipLevel = i;
        mip.data.resize(nextW * nextH * channels);

        if (isSRGB)
        {
            stbir_resize_uint8_srgb(
                mips.back().data.data(), currentW, currentH, 0,
                mip.data.data(), nextW, nextH, 0,
                layout
            );
        }
        else if (isNormal)
        {
            stbir_resize_uint8_linear(
                mips.back().data.data(), currentW, currentH, 0,
                mip.data.data(), nextW, nextH, 0,
                layout
            );

            for (int p = 0; p < nextW * nextH; p++)
            {
                // unpack
                float x = mip.data[p * 3 + 0] / 127.5f - 1.0f;
                float y = mip.data[p * 3 + 1] / 127.5f - 1.0f;
                float z = mip.data[p * 3 + 2] / 127.5f - 1.0f;

                // normalize
                float len = std::sqrt(x * x + y * y + z * z);
                if (len > 0.0001f) { x /= len; y /= len; z /= len; }

                // pack
                mip.data[p * 3 + 0] = static_cast<uint8_t>((x + 1.0f) * 127.5f);
                mip.data[p * 3 + 1] = static_cast<uint8_t>((y + 1.0f) * 127.5f);
                mip.data[p * 3 + 2] = static_cast<uint8_t>((z + 1.0f) * 127.5f);
            }
        }
        else
        {
            stbir_resize_uint8_linear(
                mips.back().data.data(), currentW, currentH, 0,
                mip.data.data(), nextW, nextH, 0,
                layout
            );
        }
        
        mips.push_back(std::move(mip));

        currentW = nextW;
        currentH = nextH;
    }

    return mips;
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

    // TODO: Use import settings instead of this approach
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

    unsigned int mipCount = CalculateMipCount(w,h);
    auto mips = GenerateMipChain(pixels, w, h, loadedChannels, mipCount, forceSRGB, isNormal);

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
    header.mipCount = mipCount;
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
    for (auto& mip : mips)
    {
        file.write(reinterpret_cast<const char*>(mip.data.data()), mip.data.size());
    }

    stbi_image_free(pixels);
    return true;
}

std::string TextureImporter::GetDefaultSettings() const
{
    TextureImportSettings s;
    json j;
    j["sRGB"] = s.sRGB;
    j["generateMips"] = s.generateMips;
    j["maxSize"] = s.maxSize;
    return j.dump();
}
