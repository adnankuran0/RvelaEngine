#include "rvelapch.h"
#include "AssetLoader.h"


Ref<Asset> AssetLoader::Load(const std::filesystem::path& path)
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
        AssetHeader header = ReadHeader(inFile, MAGIC_TEXTURE);
        std::unique_ptr<TextureMeta> meta = ReadMeta<TextureMeta>(inFile, header);

        Ref<TextureAsset> asset = CreateRef<TextureAsset>(path.string(), std::move(meta));
        if (asset->Load())
        {
            return Ref<Asset>(static_cast<Asset*>(asset.Get()));
        }
    }

    //Material Asset
#if 0
    if (ext == ".rmat")
    {
        AssetHeader header = ReadHeader(inFile, MAGIC_MATERIAL);
        std::unique_ptr<MaterialMeta> meta = ReadMeta<MaterialMeta>(inFile, header);

        Ref<MaterialAsset> asset = CreateRef<MaterialAsset>(path.string(), std::move(meta));
        if (asset->Load())
        {
            return Ref<Asset>(static_cast<Asset*>(asset.Get()));
        }
    }
#endif
    return nullptr;
}

AssetHeader AssetLoader::ReadHeader(std::ifstream& inFile, uint32_t expectedMagic)
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