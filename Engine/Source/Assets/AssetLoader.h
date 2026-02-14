#pragma once
#include <filesystem>

#include "Core/Ref.h"
#include "AssetMeta.h"
#include <fstream>
#include "Core/Log.h"

class Asset;

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

   /* template<typename T>
    static AssetUUID ReadUUID(const std::string& path)
    {
        std::ifstream inFile(path, std::ios::binary);
        if (!inFile.is_open())
        {
            LOG_ERROR("File could not be opened: {}", path);
            return nullptr; 
        }

        AssetHeader header = ReadHeader(inFile, AssetMagic<T>::magic);
        if (header.magic != AssetMagic<T>::magic)
        {
            LOG_ERROR("Magic mismatch in file: {}", path);
            return nullptr;
        }

        std::unique_ptr<T> meta = ReadMeta<T>(inFile, header);
        if (!meta)
        {
            LOG_ERROR("Meta could not be read from file: {}", path);
            return nullptr;
        }

        return meta->uuid;
    }*/

    
};