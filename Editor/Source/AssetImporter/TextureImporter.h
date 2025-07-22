#pragma once

#include "AssetImporter.h"
#include "stb_image/stb_image.h"

class TextureImporter : public IAssetImporter
{
public:
	bool Import(const std::filesystem::path& path) override;

};