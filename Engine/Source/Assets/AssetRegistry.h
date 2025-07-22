#pragma once
#include <unordered_map>
#include <filesystem>
#include "AssetUUID.h"
#include "Asset.h"
#include "AssetMeta.h"
#include "TextureAsset.h"

class AssetRegistry 
{
public:
    void Init(const std::filesystem::path& assetDir);
    

    static Ref<Asset> GetAsset(AssetUUID uuid); 
    static std::filesystem::path GetAssetPath(AssetUUID uuid);

private:
    static void ScanAssets(const std::filesystem::path& dir);
    

    static Ref<Asset> LoadAssetFromFile(const std::filesystem::path& path) {

        auto ext = path.extension().string();

        if (ext == ".rtex") 
        {
            std::ifstream inFile(path, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr << "Failed to open file: " << path << std::endl;
            }

            //header and meta
            AssetHeader header{};
            inFile.read(reinterpret_cast<char*>(&header), sizeof(header));
            if (!inFile) {
                std::cerr << "Failed to read AssetHeader\n";
            }

            if (header.magic != MAGIC_TEXTURE) {
                std::cerr << "Invalid file magic\n";
            }

            //Read meta
            std::vector<char> metaBuffer(header.metaSize);
            inFile.read(metaBuffer.data(), header.metaSize);
            if (!inFile) {
                std::cerr << "Failed to read metadata\n";
            }

            std::unique_ptr<TextureMeta> meta = std::make_unique<TextureMeta>();
            if (header.metaSize == sizeof(TextureMeta))
            {
                std::memcpy(meta.get(), metaBuffer.data(), sizeof(TextureMeta));
            }

            auto asset = CreateRef<TextureAsset>(path.string(), std::move(meta));
            if (asset->Load()) {
                return Ref<Asset>(static_cast<Asset*>(asset.Get()));
            }
        }

        return nullptr; 
    }

    inline static std::filesystem::path s_AssetDirectory;
    inline static std::unordered_map<AssetUUID, std::filesystem::path> s_UUIDToPath;
    inline static std::unordered_map<AssetUUID, Ref<Asset>> s_LoadedAssets;
};
