#pragma once

#include "AssetImporter.h"

class TextureImporter : public IAssetImporter
{
public:
	AssetUUID Import(const std::filesystem::path& path) override;
};