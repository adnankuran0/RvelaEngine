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

        std::ifstream file(m_Path, std::ios::binary);
        if (!file)
        {
            LOG_ERROR("Failed to open prefab file: {}", m_Path);
            return false;
        }

        AssetHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file)
        {
            LOG_ERROR("Failed to read prefab header");
            return false;
        }

        if (header.magic != MAGIC_PREFAB)
        {
            LOG_ERROR("Invalid prefab magic number");
            return false;
        }

        std::vector<char> metaBuffer(header.metaSize);
        file.read(metaBuffer.data(), header.metaSize);
        if (!file)
        {
            LOG_ERROR("Failed to read prefab meta");
            return false;
        }
        size_t metaOffset = 0;
        meta->Deserialize(metaBuffer, metaOffset);

        file.seekg(header.dataOffset, std::ios::beg);
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        if (fileSize < header.dataOffset)
        {
            LOG_ERROR("Invalid prefab file size");
            return false;
        }

        size_t dataSize = fileSize - header.dataOffset;
        m_Data.resize(dataSize);

        file.seekg(header.dataOffset, std::ios::beg);
        file.read(reinterpret_cast<char*>(m_Data.data()), dataSize);
        if (!file)
        {
            LOG_ERROR("Failed to read prefab data");
            return false;
        }

        m_Loaded = true;
        return true;
    }

    void Serialize()
    {
        PrefabMeta* meta = GetMetaAs<PrefabMeta>();
        if (!meta)
        {
            LOG_ERROR("Failed to get PrefabMeta");
            return;
        }

        std::vector<char> metaBuffer;
        size_t metaOffset = 0;
        meta->Serialize(metaBuffer, metaOffset);

        AssetHeader header;
        header.magic = MAGIC_PREFAB;         
        header.version = 1;
        header.type = AssetType::Prefab;
        header.metaSize = static_cast<uint32_t>(metaBuffer.size());
        header.dataOffset = sizeof(AssetHeader) + header.metaSize;
        header.reserved = 0;

        std::ofstream out(m_Path, std::ios::binary);
        if (!out)
        {
            LOG_ERROR("Failed to write prefab file: {}", m_Path);
            return;
        }

        out.write(reinterpret_cast<const char*>(&header), sizeof(header));

        out.write(metaBuffer.data(), metaBuffer.size());

        if (!m_Data.empty())
            out.write(reinterpret_cast<const char*>(m_Data.data()), m_Data.size());

        out.close();
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

    void SetData(std::vector<uint8_t>&& buffer)
    {
        m_Data = std::move(buffer);
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
