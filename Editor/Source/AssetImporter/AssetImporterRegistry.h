#pragma once
#include "Assets/Asset.h"
#include <unordered_map>

class IAssetImporter;

class AssetImporterRegistry
{
public:
	AssetImporterRegistry();
	AssetUUID Import(const std::filesystem::path& assetPath) noexcept;

private:
	AssetImporterRegistry(const AssetImporterRegistry&) = delete;
	AssetImporterRegistry& operator=(const AssetImporterRegistry&) = delete;

	void RegisterImporter(const std::vector<std::string>& extensions, std::shared_ptr<IAssetImporter> importer)
	{
		for (const auto& ext : extensions)
			m_ImporterMap[ext] = importer;
	}

private:
	std::unordered_map<std::string, std::shared_ptr<IAssetImporter>> m_ImporterMap;
};
