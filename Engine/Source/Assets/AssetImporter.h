#pragma once
#include <string>

class IAssetImporter
{
public:
	virtual bool Import(std::filesystem::path path) = 0;
	virtual bool IsExtensionValid(std::string extension) = 0;
};