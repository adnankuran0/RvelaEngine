#pragma once
#include "AssetImporter.h"
class ModelImporter : public IAssetImporter
{
	bool Import(const std::filesystem::path& path) override;
};

