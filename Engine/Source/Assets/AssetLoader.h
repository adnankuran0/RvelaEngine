#pragma once
#include <filesystem>
#include "Core/Ref.h"
#include "Assets/Asset.h"
#include "Core/Log.h"
#include "TextureAsset.h"
#include "MaterialAsset.h"

class AssetLoader
{
public:
    static Ref<Asset> Load(const std::filesystem::path& path);
    static AssetHeader ReadHeader(std::ifstream& inFile, uint32_t expectedMagic);

    template <typename T>
    inline static std::unique_ptr<T> ReadMeta(std::ifstream& inFile, const AssetHeader& header)
    {
        static_assert(std::is_base_of<AssetMeta, T>::value, "T must derive from AssetMeta");

        std::vector<char> metaBuffer(header.metaSize);
        inFile.read(metaBuffer.data(), header.metaSize);
        if (!inFile)
        {
            LOG_ERROR("Failed to read metadata");
            return nullptr;
        }

        std::unique_ptr<T> meta = std::make_unique<T>();

        size_t offset = 0;
        meta->Deserialize(metaBuffer, offset);

        return meta;
    }
};