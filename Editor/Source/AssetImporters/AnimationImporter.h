// AnimationImporter.h
#pragma once
#include "Asset/IAssetImporter.h"
#include <unordered_map>

struct aiScene;

namespace rv {

class AssetRegistry;

class AnimationImporter : public IAssetImporter
{
public:
    std::string GetImporterID() const override { return "AnimationImporter"; }

    std::vector<std::string> GetSupportedExtensions() const override { return { ".fbx", ".gltf", ".glb" }; }

    std::filesystem::path GetCachePath(const std::filesystem::path&, const AssetMeta&, const std::filesystem::path&) const override { return ""; }
    bool Import(const std::filesystem::path&, const std::filesystem::path&, const std::string&) override { return false; }

    AssetUUID ImportFromScene(
        const aiScene* scene,
        const std::filesystem::path& modelPath,
        AssetRegistry& registry,
        const std::unordered_map<std::string, int32_t>& boneNameToIndex);

    std::string GetDefaultSettings() const override { return ""; }
};

}