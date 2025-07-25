#include "rvelapch.h"
#include "AssetRegistry.h"
#include "Core/Log.h"

void AssetRegistry::Init(const std::filesystem::path& assetDir)
{
    s_AssetDirectory = assetDir;
    ScanAssets(assetDir);
}

std::filesystem::path AssetRegistry::GetAssetPath(const AssetUUID& uuid)
{
    if (IsExist(uuid))
        return s_UUIDToPath[uuid];
    return {};
}

void AssetRegistry::ScanAssets(const std::filesystem::path& dir)
{
    for (auto& entry : std::filesystem::recursive_directory_iterator(dir))
    {
        if (!entry.is_regular_file()) continue;

        auto path = entry.path();

        //Texture Asset
        if (path.extension() == ".rtex")
        {
            std::ifstream inFile(path, std::ios::binary);
            if (!inFile.is_open()) 
            {
                LOG_ERROR("Failed to open file: {}", path.string());
                continue;
            }

            AssetHeader header = ReadHeader(inFile, MAGIC_TEXTURE);
            std::unique_ptr<TextureMeta> meta = ReadMeta<TextureMeta>(inFile,header);

            LOG_INFO("Asset found with uuid: {}", meta->uuid.ToString());
            s_UUIDToPath[meta->uuid] = path;
        }
    }
}

Ref<Asset> AssetRegistry::LoadAssetFromFile(const std::filesystem::path& path)
{
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile.is_open())
    {
        LOG_ERROR("Failed to open file: {}", path.string());
    }

    auto ext = path.extension().string();

    //Texture Asset
    if (ext == ".rtex")
    {
        AssetHeader header = ReadHeader(inFile,MAGIC_TEXTURE);
        std::unique_ptr<TextureMeta> meta = ReadMeta<TextureMeta>(inFile,header);

        Ref<TextureAsset> asset = CreateRef<TextureAsset>(path.string(), std::move(meta));
        if (asset->Load()) 
        {
            return Ref<Asset>(static_cast<Asset*>(asset.Get()));
        }
    }

    //Material Asset
    if (ext == ".rmat")
    {

    }

    return nullptr;
}


AssetHeader AssetRegistry::ReadHeader(std::ifstream& inFile, uint32_t expectedMagic)
{
    if (!inFile) 
    {
        LOG_ERROR("Failed to read AssetHeader");
    }
    AssetHeader header{};
    inFile.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.magic != expectedMagic) 
    {
        LOG_ERROR("Invalid file magic");
    }

    return header;
}