#include "rvelapch.h"
#include "AssetLoader.h"
#include "stb_image_write.h"
#include "MeshAsset.h"
#include "PrefabAsset.h"

#include "Assets/Asset.h"
#include "Core/Log.h"
#include "TextureAsset.h"
#include "MaterialAsset.h"
#include "Assets/AssetMagic.h"

Ref<Asset> AssetLoader::Load(const std::filesystem::path& path)
{
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile.is_open())
    {
        LOG_ERROR("[AssetLoader::Load] Failed to open file: {}", path.string());
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
            return Ref<Asset>(asset);
        }
    }

    //Material Asset
    if (ext == ".rmat")
    {
        AssetHeader header = ReadHeader(inFile, MAGIC_MATERIAL);
        std::unique_ptr<MaterialMeta> meta = ReadMeta<MaterialMeta>(inFile, header);

        Ref<MaterialAsset> asset = CreateRef<MaterialAsset>(path.string(), std::move(meta));
        if (asset->Load())
        {
            return Ref<Asset>(asset);
        }
    }

    //Mesh Asset
    if (ext == ".rmesh")
    {
        AssetHeader header = ReadHeader(inFile, MAGIC_MESH);
        std::unique_ptr<MeshMeta> meta = ReadMeta<MeshMeta>(inFile, header);

        Ref<MeshAsset> asset = CreateRef<MeshAsset>(path.string(), std::move(meta));
        if (asset->Load())
        {
            return Ref<Asset>(asset);
        }
    }

    //Mesh Asset
    if (ext == ".rprefab")
    {
        AssetHeader header = ReadHeader(inFile, MAGIC_PREFAB);
        std::unique_ptr<PrefabMeta> meta = ReadMeta<PrefabMeta>(inFile, header);

        Ref<PrefabAsset> asset = CreateRef<PrefabAsset>(path.string(), std::move(meta));
        if (asset->Load())
        {
            return Ref<Asset>(asset);
        }
    }
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