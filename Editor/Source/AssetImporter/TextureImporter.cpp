#include "rvelapch.h"
#include "TextureImporter.h"
#include "Assets/TextureAsset.h"

bool TextureImporter::Import(const std::filesystem::path& path)
{
	int width, height, channels;
	uint8_t* textureData = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
	if (!textureData)
	{
		LOG_ERROR("Failed to load image: {}", path.string());
		return false;
	}

	TextureMeta meta;
	meta.sourcePath = path;
	meta.type = AssetType::Texture;
	meta.width = width;
	meta.height = height;
	meta.mipCount = 1;
	meta.format = (channels == 3) ? TextureFormat::RGB8 :
		(channels == 4) ? TextureFormat::RGBA8 :
		TextureFormat::Unknown;
	meta.isSRGB = false;

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

	outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
	outFile.write(reinterpret_cast<const char*>(metaBuffer.data()), metaBuffer.size());
	outFile.write(reinterpret_cast<const char*>(textureData), width * height * 4);
	outFile.close();

	stbi_image_free(textureData);
	LOG_INFO("Texture asset imported with UUID: {}", meta.uuid.ToString());
	return true;
}


