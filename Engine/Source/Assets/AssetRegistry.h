#pragma once
#include <unordered_map>
#include <filesystem>
#include "AssetUUID.h"
#include "Asset.h"
#include "AssetMeta.h"
#include "TextureAsset.h"

class AssetRegistry {
public:
    static void Initialize(const std::filesystem::path& assetDir) {
        s_AssetDirectory = assetDir;
        ScanAssets(assetDir);
    }

    static Ref<Asset> GetAsset(AssetUUID uuid) {
        if (s_LoadedAssets.contains(uuid))
            return s_LoadedAssets[uuid];

        if (!s_UUIDToPath.contains(uuid))
            return nullptr;

        auto path = s_UUIDToPath[uuid];
        Ref<Asset> asset = LoadAssetFromFile(path);
        if (asset)
            s_LoadedAssets[uuid] = asset;

        return asset;
    }

    static std::filesystem::path GetAssetPath(AssetUUID uuid) {
        if (s_UUIDToPath.contains(uuid))
            return s_UUIDToPath[uuid];
        return {};
    }

private:
    static void ScanAssets(const std::filesystem::path& dir) {
        for (auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;

            auto path = entry.path();
            if (path.extension() == ".meta") {
                try {
                    // read meta header
                    std::filesystem::path assetPath = path;

                    AssetMeta meta;
                    s_UUIDToPath[meta.uuid] = assetPath;
                }
                catch (...) {
                }
            }
        }
    }

    static Ref<Asset> LoadAssetFromFile(const std::filesystem::path& path) {

        auto ext = path.extension().string();

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
            auto asset = CreateRef<TextureAsset>(path.string());
            if (asset->Load()) {
                return Ref<Asset>(static_cast<Asset*>(asset.Get()));
            }
        }

        return nullptr; // TODO: Asset loading
    }

    inline static std::filesystem::path s_AssetDirectory;
    inline static std::unordered_map<AssetUUID, std::filesystem::path> s_UUIDToPath;
    inline static std::unordered_map<AssetUUID, Ref<Asset>> s_LoadedAssets;
};
