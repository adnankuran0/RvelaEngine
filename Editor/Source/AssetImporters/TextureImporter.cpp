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
#include <array>

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
    settings.compress = j.value("compress", true);
    return settings;
}

DXGI_FORMAT TextureImporter::ToDXGI(TextureFormat format, bool sRGB)
{
    switch (format)
    {
    case TextureFormat::BC1:
        return sRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
    case TextureFormat::BC3:
        return sRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
    case TextureFormat::BC4:
        return DXGI_FORMAT_BC4_UNORM;
    case TextureFormat::BC5:
        return DXGI_FORMAT_BC5_UNORM;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
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

std::vector<DirectX::Image> TextureImporter::BuildImages(
    const std::vector<TextureImporter::Mip>& mips,
    int channels,
    std::vector<std::vector<uint8_t>>& expandedBuffers)
{
    std::vector<DirectX::Image> images;
    images.resize(mips.size());

    DXGI_FORMAT srcFormat;
    size_t bytesPerPixel;

    switch (channels)
    {
    case 1:
        srcFormat = DXGI_FORMAT_R8_UNORM;
        bytesPerPixel = 1;
        break;
    case 2:
        srcFormat = DXGI_FORMAT_R8G8_UNORM;
        bytesPerPixel = 2;
        break;
    case 3: // expand
    case 4:
        srcFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        bytesPerPixel = 4;
        break;
    default:
        assert(false);
        return {};
    }

    for (size_t i = 0; i < mips.size(); i++)
    {
        const auto& m = mips[i];

        images[i].width = m.w;
        images[i].height = m.h;
        images[i].format = srcFormat;

        if (channels == 3)
        {
            auto& expanded = expandedBuffers.emplace_back(m.w * m.h * 4);
            for (int p = 0; p < m.w * m.h; p++)
            {
                expanded[p * 4 + 0] = m.data[p * 3 + 0];
                expanded[p * 4 + 1] = m.data[p * 3 + 1];
                expanded[p * 4 + 2] = m.data[p * 3 + 2];
                expanded[p * 4 + 3] = 255;
            }
            images[i].pixels = expanded.data();
            images[i].rowPitch = m.w * 4;
            images[i].slicePitch = m.w * m.h * 4;
        }
        else
        {
            images[i].pixels = const_cast<uint8_t*>(m.data.data());
            images[i].rowPitch = m.w * bytesPerPixel;
            images[i].slicePitch = m.w * m.h * bytesPerPixel;
        }
    }

    return images;
}

std::vector<uint8_t> TextureImporter::CompressMips(const std::vector<TextureImporter::Mip>& mips, 
    int channels, TextureFormat format, bool isSRGB)
{
    std::vector<std::vector<uint8_t>> expandedBuffers;
    auto images = BuildImages(mips, channels, expandedBuffers);

    DirectX::TexMetadata meta{};
    meta.width = mips[0].w;
    meta.height = mips[0].h;
    meta.mipLevels = (size_t)mips.size();
    meta.arraySize = 1;
    meta.depth = 1;
    meta.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;
    meta.format = images[0].format;

    DXGI_FORMAT dstFormat = ToDXGI(format, isSRGB);

    DirectX::TEX_COMPRESS_FLAGS flags = DirectX::TEX_COMPRESS_PARALLEL;

    if (isSRGB)
        flags |= DirectX::TEX_COMPRESS_SRGB;

    DirectX::ScratchImage compressed;

    HRESULT hr = DirectX::Compress(
        images.data(),
        images.size(),
        meta,
        dstFormat,
        flags,
        1.0f,
        compressed
    );

    if (FAILED(hr))
    {
        LOG_ERROR("Texture compression failed");
        return {};
    }

    std::vector<uint8_t> out;

    const DirectX::Image* imgs = compressed.GetImages();
    size_t count = compressed.GetImageCount();

    for (size_t i = 0; i < count; i++)
    {
        const DirectX::Image& img = imgs[i];
        out.insert(out.end(), img.pixels, img.pixels + img.slicePitch);
    }

    return out;
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

    bool isAlbedo = false;
    bool isNormal = false;
    bool hasAlpha = false;
    bool isSingleChannel = false;

    // TODO: Use import settings instead of this approach
    if (lowerFilename.find("nor") != std::string::npos ||
        lowerFilename.find("normal") != std::string::npos)
    {
        isNormal = true;
    }

    stbi_set_flip_vertically_on_load(true);
    int w, h, channels;
    uint8_t* pixels = stbi_load(sourcePath.string().c_str(), &w, &h, &channels, 0);
    if (!pixels)
    {
        LOG_ERROR("stb_image failed: {}", sourcePath.string());
        return false;
    }

    switch (channels)
    {
    case 1:
        isSingleChannel = true;
        break;
    case 3:
        if (!isNormal) isAlbedo = true;
        break;
    case 4:
        if (!isNormal) isAlbedo = true;
        hasAlpha = true;
        break;
    default:
        LOG_ERROR("Unsupported channel count: {}", channels);
        return false;
        break;
    }

    if (w > settings.maxSize || h > settings.maxSize)
        LOG_WARN("Texture exceeds maxSize ({}): {}x{} — {}",
            settings.maxSize, w, h, sourcePath.string());

    assert(!(isNormal && channels != 3) && "Normal map must be RGB!");

    unsigned int mipCount = CalculateMipCount(w,h);
    auto mips = GenerateMipChain(pixels, w, h, channels, mipCount, isAlbedo, isNormal);

    std::vector<uint8_t> finalData;

    TextureCacheHeader header{};
    TextureFormat format = TextureFormat::Unknown;
    if (!settings.compress)
    {
        switch (channels)
        {
        case 1: format = TextureFormat::R8; break;
        case 3: format = TextureFormat::RGBA8; break; // expand
        case 4: format = TextureFormat::RGBA8; break;
        }
        for (auto& mip : mips)
        {
            finalData.insert(finalData.end(), mip.data.begin(), mip.data.end());
        }
    }
    else
    {
        if (isSingleChannel) format = TextureFormat::BC4;
        else if (isNormal) format = TextureFormat::BC5;
        else if (isAlbedo)
        {
            if (hasAlpha) format = TextureFormat::BC3;
            else format = TextureFormat::BC1;
        }
        finalData = CompressMips(mips, channels, format, isAlbedo);
    }
    header.width = static_cast<uint32_t>(w);
    header.height = static_cast<uint32_t>(h);
    header.format = format;
    header.mipCount = mipCount;
    header.isSRGB = isAlbedo ? 1 : 0;
    header.dataSize = (uint32_t)finalData.size();

    size_t pixelSize = static_cast<size_t>(w) * h * channels;
   
    std::ofstream file(outCachePath, std::ios::binary);
    if (!file)
    {
        stbi_image_free(pixels);
        LOG_ERROR("Cannot write cache: {}", outCachePath.string());
        return false;
    }

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(finalData.data()), header.dataSize);

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
    j["compress"] = s.compress;
    return j.dump();
}
