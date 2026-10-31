#pragma once
#include "Asset/IAssetLoader.h"
#include <nlohmann/json.hpp>

namespace rv {

class SkeletonLoader : public IAssetLoader
{
public:
    std::string GetLoaderID() const override { return "SkeletonLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rskeleton" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
    Ref<Asset> LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta) override;

private:
    Ref<Asset> ParseSkeleton(const nlohmann::json& j, const AssetMeta& meta, const std::string& debugPath);
};

}
