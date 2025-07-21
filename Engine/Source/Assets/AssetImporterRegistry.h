#pragma once
#include "Asset.h"
#include "AssetImporters/TextureImporter.h"

class AssetImporterRegistry
{
public:
	static AssetImporterRegistry& Get()
	{
		static AssetImporterRegistry instance;
		return instance;
	}

	bool Import(const std::filesystem::path& assetPath)
	{
		//choose the right importer.
		std::string extension = assetPath.extension().string();
		if (m_TextureImporter.IsExtensionValid(extension))
		{
			// import texture
			return true;
		}
		else
		{
			return false;
		}
	}

private:
	AssetImporterRegistry() = default;
	~AssetImporterRegistry() = default;

	AssetImporterRegistry(const AssetImporterRegistry&) = delete;
	AssetImporterRegistry& operator=(const AssetImporterRegistry&) = delete;

	TextureImporter m_TextureImporter;
};
