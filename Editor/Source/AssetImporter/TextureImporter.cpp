#include "rvelapch.h"
#include "TextureImporter.h"
#include "Assets/TextureAsset.h"

bool TextureImporter::Import(const std::filesystem::path& path)
{
    int width, height, originalChannels;

    if (!stbi_info(path.string().c_str(), &width, &height, &originalChannels))
    {
        LOG_ERROR("Failed to read image info: {}", path.string());
        return false;
    }

    int desiredChannels = originalChannels; // Or 0 to keep original channels

    int loadWidth, loadHeight, actualChannels;
    uint8_t* textureData = stbi_load(path.string().c_str(), &loadWidth, &loadHeight, &actualChannels, desiredChannels);

    if (!textureData)
    {
        LOG_ERROR("Failed to load image: {}", path.string());
        return false;
    }

    if (loadWidth != width || loadHeight != height)
        LOG_WARN("Loaded dimensions differ from stbi_info.");

    TextureFormat format;
    bool isSRGB;

    switch (actualChannels)
    {
    case 4:
        format = TextureFormat::RGBA8;
        isSRGB = true; // Albedo gibi, genellikle sRGB
        break;
    case 3:
        format = TextureFormat::RGB8;
        isSRGB = false; // Normal map gibi linear
        break;
    case 1:
        format = TextureFormat::R8;
        isSRGB = false; // Roughness, AO, linear
        break;
    default:
        LOG_ERROR("Unsupported channel count: {}", actualChannels);
        stbi_image_free(textureData);
        return false;
    }

    TextureMeta meta;
    meta.sourcePath = path;
    meta.type = AssetType::Texture;
    meta.width = width;
    meta.height = height;
    meta.mipCount = 1;
    meta.format = format;
    meta.isSRGB = isSRGB ? 1 : 0;

    std::vector<char> metaBuffer;
    size_t offset = 0;
    meta.Serialize(metaBuffer, offset);

    AssetHeader header;
    header.magic = MAGIC_TEXTURE;
    header.version = 1;
    header.type = AssetType::Texture;
    header.metaSize = static_cast<uint32_t>(metaBuffer.size());
    header.dataOffset = sizeof(AssetHeader) + header.metaSize;
    header.reserved = 0;

    std::filesystem::path outputPath = path;
    outputPath.replace_extension(".rtex");

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile)
    {
        LOG_ERROR("Failed to open output file.");
        stbi_image_free(textureData);
        return false;
    }

    int dataSize = width * height * actualChannels;

    outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
    outFile.write(reinterpret_cast<const char*>(metaBuffer.data()), metaBuffer.size());
    outFile.write(reinterpret_cast<const char*>(textureData), dataSize);
    outFile.close();

    stbi_image_free(textureData);

    LOG_INFO("Texture imported: {} ({}x{} {}ch) UUID: {}",
        path.filename().string(), width, height, actualChannels, meta.uuid.ToString());

    return true;
}