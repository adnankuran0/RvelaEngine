#include "rvelapch.h"
#include "TextureImporter.h"

bool TextureImporter::Import(std::filesystem::path path)
{
	std::cout << "Texture asset imported!" << std::endl;
	return true;
}

bool TextureImporter::IsExtensionValid(std::string extension)
{
	return std::find(m_SupportedExtensions.begin(), m_SupportedExtensions.end(), extension) != m_SupportedExtensions.end();
}