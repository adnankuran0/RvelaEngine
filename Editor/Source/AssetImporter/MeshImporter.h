#pragma once
#include "AssetImporter.h"

class MeshImporter : public IAssetImporter
{
public:
	bool Import(const std::filesystem::path& path) override;
};