#pragma once
#include <string>
#include <filesystem>

class IAssetImporter
{
public:
	virtual bool Import(const std::filesystem::path& path) = 0;
};