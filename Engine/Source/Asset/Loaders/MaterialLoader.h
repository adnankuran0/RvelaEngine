#pragma once
#include "Asset/IAssetLoader.h"

namespace rv {

class MaterialAsset;

class MaterialLoader : public IAssetLoader
{
public:
    MaterialLoader();
    std::string GetLoaderID() const override { return "MaterialLoader"; }
    std::vector<std::string> GetSupportedExtensions() const override { return { ".rmat" }; }
    Ref<Asset> Load(const std::filesystem::path& assetPath, const AssetMeta& meta) override;
    const Ref<MaterialAsset> GetDefaultMaterial() { return s_DefaultMaterial; }
private:
    inline static Ref<MaterialAsset> s_DefaultMaterial;
};

}