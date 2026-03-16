#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Core/Ref.h"

namespace rv {

class Asset;
struct AssetMeta;

class IAssetLoader
{
public:
    virtual ~IAssetLoader() = default;

    virtual std::string GetLoaderID() const = 0;
    virtual std::vector<std::string> GetSupportedExtensions() const = 0;
    virtual Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) = 0;
};

}