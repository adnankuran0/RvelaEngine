#pragma once
#include "Assets/AssetMeta.h"
#include "Assets/Asset.h"
#include "Core/Log.h"
#include <fstream>

class PrefabMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override
    {
        return std::make_unique<PrefabMeta>(*this);
    }

    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override
    {
        SerializeBase(outBuffer, offset); // UUID, sourcePath, type

        if (outBuffer.size() < offset + sizeof(entityCount))
            outBuffer.resize(offset + sizeof(entityCount));

        memcpy(outBuffer.data() + offset, &entityCount, sizeof(entityCount));
        offset += sizeof(entityCount);
    }

    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override
    {
        DeserializeBase(inBuffer, offset);

        memcpy(&entityCount, inBuffer.data() + offset, sizeof(entityCount));
        offset += sizeof(entityCount);
    }

public:
    uint32_t entityCount = 0;
};

class PrefabAsset : public Asset
{
public:
    PrefabAsset(const std::string& path, std::unique_ptr<PrefabMeta> prefabMeta)
        : m_Path(path), m_Loaded(false), Asset(std::move(prefabMeta))
    {
    }

    ~PrefabAsset()
    {
        Unload();
    }

    bool Load()
    {
        if (m_Loaded) return true;

        PrefabMeta* meta = GetMetaAs<PrefabMeta>();
        if (!meta)
        {
            LOG_ERROR("Failed to get PrefabMeta");
            return false;
        }

        m_Data = ReadPrefabData(m_Path);
        if (m_Data.empty())
        {
            LOG_ERROR("Failed to read prefab data from file");
            return false;
        }

        m_Loaded = true;
        return true;
    }

    void Unload()
    {
        m_Data.clear();
        m_Loaded = false;
    }

    const std::vector<uint8_t>& GetData() const
    {
        return m_Data;
    }

    const std::string& GetPath() const
    {
        return m_Path;
    }

    bool IsLoaded() const
    {
        return m_Loaded;
    }

private:
    static std::vector<uint8_t> ReadPrefabData(const std::filesystem::path& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file)
        {
            LOG_ERROR("Failed to open file: {}", filePath.string());
            return {};
        }

        AssetHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        size_t dataOffset = sizeof(AssetHeader) + header.metaSize;

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        if (fileSize < dataOffset)
        {
            LOG_ERROR("Invalid prefab file size.");
            return {};
        }

        size_t dataSize = fileSize - dataOffset;
        std::vector<uint8_t> data(dataSize);

        file.seekg(dataOffset, std::ios::beg);
        file.read(reinterpret_cast<char*>(data.data()), dataSize);
        file.close();

        return data;
    }

private:
    std::string m_Path;
    std::vector<uint8_t> m_Data;
    bool m_Loaded;
};
