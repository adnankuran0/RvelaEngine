#pragma once
#include "Asset/IAssetLoader.h"
#include <nlohmann/json.hpp>

namespace rv {

class MaterialAsset;

class MaterialLoader : public IAssetLoader
{
public:
    MaterialLoader();
    std::string GetLoaderID() const override { return "MaterialLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rmat" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
    Ref<Asset> LoadFromMemory(const uint8_t* data, size_t size, const AssetMeta& meta) override;
    static const Ref<MaterialAsset>& GetDefaultMaterial() { return s_DefaultMaterial; }

private:
    Ref<Asset> ParseMaterial(const nlohmann::json& j, const AssetMeta& meta);
    inline static Ref<MaterialAsset> s_DefaultMaterial;
};

}
