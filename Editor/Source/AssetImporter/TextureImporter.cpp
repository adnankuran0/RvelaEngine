#include "rvelapch.h"
#include "TextureImporter.h"
#include "Assets/TextureAsset.h"

bool TextureImporter::Import(const std::filesystem::path& path)
{
    //Read original file info
    int width, height, originalChannels;
    if (!stbi_info(path.string().c_str(), &width, &height, &originalChannels))
    {
        LOG_ERROR("Failed to read image info: {}", path.string());
        return false;
    }

    std::string filename = path.filename().string();
    std::string lowerFilename = filename;
    std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);

    bool force4Channels = false;
    bool force3Channels = false;
    bool force1Channel = false;
    bool isSRGB = false;
    bool isAlbedo = false;

    if (lowerFilename.find("ao") != std::string::npos ||
        lowerFilename.find("rough") != std::string::npos ||
        lowerFilename.find("metal") != std::string::npos)
    {
        force1Channel = true;
        isSRGB = false;
    }
    else if (lowerFilename.find("diff") != std::string::npos ||
        lowerFilename.find("albedo") != std::string::npos ||
        lowerFilename.find("basecolor") != std::string::npos)
    {
        isAlbedo = true;
        isSRGB = false;  

        force4Channels = true;
    }
    else if (lowerFilename.find("nor") != std::string::npos ||
        lowerFilename.find("normal") != std::string::npos)
    {
        force3Channels = true;
        isSRGB = false;
    }
    else
    {
        isSRGB = (originalChannels >= 3);
        if (originalChannels == 1) force1Channel = true;
        else if (originalChannels == 2) force3Channels = true; 
        else if (originalChannels == 3) force3Channels = true;
        else if (originalChannels >= 4) force4Channels = true;
    }

    int desiredChannels = 0;
    if (force4Channels) desiredChannels = 4;
    else if (force3Channels) desiredChannels = 3;
    else if (force1Channel) desiredChannels = 1;

    int loadWidth, loadHeight, actualChannels;
    uint8_t* textureData = stbi_load(path.string().c_str(), &loadWidth, &loadHeight, &actualChannels, desiredChannels);
    //stbi_write_png("debug_imported.png", loadWidth, loadHeight, desiredChannels, textureData, loadWidth * desiredChannels);

    if (!textureData)
    {
        LOG_ERROR("Failed to load image data: {}", path.string());
        return false;
    }

    if (loadWidth != width || loadHeight != height)
        LOG_WARN("Loaded dimensions differ from stbi_info.");

    TextureFormat format;
    int usedChannels = desiredChannels > 0 ? desiredChannels : actualChannels;

    
    switch (usedChannels)
    {
    case 4: format = TextureFormat::RGBA8; break;
    case 3: format = TextureFormat::RGB8; break;
    case 1: format = TextureFormat::R8; break;
    default:
        LOG_ERROR("Unsupported channel count: {}", usedChannels);
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

    int dataSize = width * height * usedChannels;

    outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
    outFile.write(reinterpret_cast<const char*>(metaBuffer.data()), metaBuffer.size());
    outFile.write(reinterpret_cast<const char*>(textureData), dataSize);
    outFile.close();

    stbi_image_free(textureData);

    LOG_INFO("Texture imported: {} ({}x{} {}ch) UUID: {}",
        path.filename().string(), width, height, usedChannels, meta.uuid.ToString());

    return true;
}