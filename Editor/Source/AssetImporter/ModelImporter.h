#pragma once
#include "AssetImporter.h"
#include "MeshImporter.h"
class ModelImporter : public IAssetImporter
{
	bool Import(const std::filesystem::path& path) override;
private:
	MeshImporter m_MeshImporter;

};

