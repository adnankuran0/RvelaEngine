#pragma once
#include <string>
#include <filesystem>
#include "Assets/AssetUUID.h"

class IAssetImporter
{
public:
	virtual AssetUUID Import(const std::filesystem::path& path) = 0;
};