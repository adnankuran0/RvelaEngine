#pragma once
#include <filesystem>
#include "Assets/AssetUUID.h"

namespace rv {

class IAssetImporter
{
public:
	virtual AssetUUID Import(const std::filesystem::path& path) = 0;
};

}