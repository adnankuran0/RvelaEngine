#pragma once
#include "Assets/AssetMeta.h"
#include "Assets/Asset.h"
#include "Core/Log.h"
#include <fstream>

class SceneMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override
    {
        return std::make_unique<SceneMeta>(*this);
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

class SceneAsset : public Asset
{
public:
    SceneAsset(const std::string& path, std::unique_ptr<SceneMeta> sceneMeta)
        : m_Path(path), m_Loaded(false), Asset(std::move(sceneMeta))
    {
    }

    ~SceneAsset()
    {
        Unload();
    }

    bool Load()
    {
        if (m_Loaded) return true;

        SceneMeta* meta = GetMetaAs<SceneMeta>();
        if (!meta)
        {
            LOG_ERROR("Failed to get SceneMeta");
            return false;
        }

        std::ifstream file(m_Path, std::ios::binary);
        if (!file)
        {
            LOG_ERROR("Failed to open scene file: {}", m_Path);
            return false;
        }

        AssetHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!file)
        {
            LOG_ERROR("Failed to read scene header");
            return false;
        }

        if (header.magic != MAGIC_SCENE)
        {
            LOG_ERROR("Invalid scene magic number");
            return false;
        }

        std::vector<char> metaBuffer(header.metaSize);
        file.read(metaBuffer.data(), header.metaSize);
        if (!file)
        {
            LOG_ERROR("Failed to read scene meta");
            return false;
        }
        size_t metaOffset = 0;
        meta->Deserialize(metaBuffer, metaOffset);

        file.seekg(header.dataOffset, std::ios::beg);
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        if (fileSize < header.dataOffset)
        {
            LOG_ERROR("Invalid scene file size");
            return false;
        }

        size_t dataSize = fileSize - header.dataOffset;
        m_Data.resize(dataSize);

        file.seekg(header.dataOffset, std::ios::beg);
        file.read(reinterpret_cast<char*>(m_Data.data()), dataSize);
        if (!file)
        {
            LOG_ERROR("Failed to read scene data");
            return false;
        }

        m_Loaded = true;
        return true;
    }

    void Serialize()
    {
        SceneMeta* meta = GetMetaAs<SceneMeta>();
        if (!meta)
        {
            LOG_ERROR("Failed to get SceneMeta");
            return;
        }

        std::vector<char> metaBuffer;
        size_t metaOffset = 0;
        meta->Serialize(metaBuffer, metaOffset);

        AssetHeader header;
        header.magic = MAGIC_SCENE;
        header.version = 1;
        header.type = AssetType::Scene;
        header.metaSize = static_cast<uint32_t>(metaBuffer.size());
        header.dataOffset = sizeof(AssetHeader) + header.metaSize;
        header.reserved = 0;

        std::ofstream out(m_Path, std::ios::binary);
        if (!out)
        {
            LOG_ERROR("Failed to write scene file: {}", m_Path);
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

    const std::vector<std::byte>& GetData() const
    {
        return m_Data;
    }

    void SetData(std::vector<std::byte>&& buffer)
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
    static std::vector<uint8_t> ReadSceneData(const std::filesystem::path& filePath)
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
            LOG_ERROR("Invalid scene file size.");
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
    std::vector<std::byte> m_Data;
    bool m_Loaded;
};
