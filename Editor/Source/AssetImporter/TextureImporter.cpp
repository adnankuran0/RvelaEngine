#include "rvelapch.h"
#include "TextureImporter.h"
#include "Assets/TextureAsset.h"

bool TextureImporter::Import(const std::filesystem::path& path)
{
	//Load data from file
	int width, height, channels;
	uint8_t* textureData = stbi_load(path.string().c_str() , &width, &height, &channels, 4);

	//Setup binary file
	AssetHeader header;
	header.magic = 0x52544558; // 'RTEX' 
	header.version = 1;
	header.type = AssetType::Texture;
	header.reserved = 0;

	TextureMeta meta;
	meta.sourcePath = path;
	meta.type = AssetType::Texture;
	meta.width = width;
	meta.height = height;
	meta.mipCount = 1;
	switch (channels) 
	{
		case 3: meta.format = TextureFormat::RGB8; break;
		case 4: meta.format = TextureFormat::RGBA8; break;
		default: meta.format = TextureFormat::Unknown; break;
	}
	meta.isSRGB = false;

	header.metaSize = sizeof(TextureMeta);
	header.dataOffset = sizeof(AssetHeader) + header.metaSize;

	//Write to file
	std::filesystem::path outputPath = path;
	outputPath.replace_extension(".rtex");
	std::ofstream outFile(outputPath , std::ios::binary);
	if (!outFile)
	{
		std::cerr << "Failed to open output file." << std::endl;
		return false;
	}

	outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
	outFile.write(reinterpret_cast<const char*>(&meta), sizeof(meta));
	outFile.write(reinterpret_cast<const char*>(textureData), width * height * 4);

	outFile.close();

	stbi_image_free(textureData);

	std::cout << "Texture asset imported!" << std::endl;
	return true;
}

