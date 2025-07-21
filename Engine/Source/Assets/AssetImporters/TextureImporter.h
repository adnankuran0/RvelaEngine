#pragma once

#include "../AssetImporter.h"


class TextureImporter : public IAssetImporter
{
public:
	bool Import(std::filesystem::path path) override;
	bool IsExtensionValid(std::string extension) override;
private:
	std::array<std::string, 4> m_SupportedExtensions{".png", ".jpg", ".jpeg", ".tga"};

};