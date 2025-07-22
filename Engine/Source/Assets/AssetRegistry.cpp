#include "rvelapch.h"
#include "AssetRegistry.h"

void AssetRegistry::Init(const std::filesystem::path& assetDir)
{
    s_AssetDirectory = assetDir;
    ScanAssets(assetDir);
}

Ref<Asset> AssetRegistry::GetAsset(AssetUUID uuid)
{
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

std::filesystem::path AssetRegistry::GetAssetPath(AssetUUID uuid)
{
    if (s_UUIDToPath.contains(uuid))
        return s_UUIDToPath[uuid];
    return {};
}

void AssetRegistry::ScanAssets(const std::filesystem::path& dir)
{
    for (auto& entry : std::filesystem::recursive_directory_iterator(dir))
    {
        if (!entry.is_regular_file()) continue;

        auto path = entry.path();
        if (path.extension() == ".rtex")
        {
            std::ifstream inFile(path, std::ios::binary);
            if (!inFile.is_open()) {
                std::cerr << "Failed to open file: " << path << std::endl;
                continue;
            }

            //header and meta
            AssetHeader header{};
            inFile.read(reinterpret_cast<char*>(&header), sizeof(header));
            if (!inFile) {
                std::cerr << "Failed to read AssetHeader\n";
                continue;
            }

            if (header.magic != MAGIC_TEXTURE) {
                std::cerr << "Invalid file magic\n";
                continue;
            }

            //Read meta
            std::vector<char> metaBuffer(header.metaSize);
            inFile.read(metaBuffer.data(), header.metaSize);
            if (!inFile) {
                std::cerr << "Failed to read metadata\n";
                continue;
            }

            std::unique_ptr<TextureMeta> meta = std::make_unique<TextureMeta>();
            if (header.metaSize == sizeof(TextureMeta))
            {
                std::memcpy(&meta->width, metaBuffer.data(), sizeof(TextureMeta::width));
                std::memcpy(&meta->height, metaBuffer.data() + sizeof(uint32_t), sizeof(TextureMeta::height));
                std::memcpy(&meta->mipCount, metaBuffer.data() + 2 * sizeof(uint32_t), sizeof(TextureMeta::mipCount));
                std::memcpy(&meta->format, metaBuffer.data() + 2 * sizeof(uint32_t) + sizeof(uint16_t), sizeof(TextureMeta::format));
                std::memcpy(&meta->isSRGB, metaBuffer.data() + 2 * sizeof(uint32_t) + sizeof(uint16_t) + sizeof(TextureFormat), sizeof(meta->isSRGB));
                std::memcpy(&meta->reserved, metaBuffer.data() + header.metaSize - sizeof(meta->reserved), sizeof(meta->reserved));
            }
            std::cout << "Asset found with uuid: " << meta->uuid.ToString() << "\n";
            s_UUIDToPath[meta->uuid] = path;
        }
    }
}