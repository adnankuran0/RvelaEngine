#pragma once
#include <filesystem>
#include "Asset/AssetUUID.h"
#include "Asset/AssetMeta.h"

namespace rv {

class IAssetImporter
{
public:
	virtual ~IAssetImporter() = default;
	virtual std::string GetImporterID() const = 0;
	virtual std::filesystem::path GetCachePath(
		const std::filesystem::path& sourcePath,
		const AssetMeta& meta,
		const std::filesystem::path& cacheRoot) const = 0;
	virtual std::vector<std::string> GetSupportedExtensions() const = 0;

	virtual bool Import(
		const std::filesystem::path& sourcePath,
		const std::filesystem::path& outCachePath,
		const std::string& settingsJson) = 0;

};
	
}