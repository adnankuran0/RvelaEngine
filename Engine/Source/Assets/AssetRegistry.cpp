#include "rvelapch.h"
#include "AssetRegistry.h"
#include "Core/Log.h"
#include "MaterialAsset.h"

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

            AssetHeader header = AssetLoader::ReadHeader(inFile, MAGIC_TEXTURE);
            std::unique_ptr<TextureMeta> meta = AssetLoader::ReadMeta<TextureMeta>(inFile,header);

            LOG_INFO("Asset found with uuid: {}", meta->uuid.ToString());
            s_UUIDToPath[meta->uuid] = path;
        }
    }
}

