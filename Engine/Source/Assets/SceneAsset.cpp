#include "rvelapch.h"
#include "SceneAsset.h"
#include "Core/Log.h"
#include <fstream>
#include <cstring>

std::unique_ptr<AssetMeta> SceneMeta::Clone() const
{
    return std::make_unique<SceneMeta>(*this);
}

void SceneMeta::Serialize(std::vector<char>& outBuffer, size_t& offset) const
{
    SerializeBase(outBuffer, offset);

    if (outBuffer.size() < offset + sizeof(entityCount))
        outBuffer.resize(offset + sizeof(entityCount));

    memcpy(outBuffer.data() + offset, &entityCount, sizeof(entityCount));
    offset += sizeof(entityCount);
}

void SceneMeta::Deserialize(const std::vector<char>& inBuffer, size_t& offset)
{
    DeserializeBase(inBuffer, offset);

    memcpy(&entityCount, inBuffer.data() + offset, sizeof(entityCount));
    offset += sizeof(entityCount);
}

SceneAsset::SceneAsset(const std::string& path, std::unique_ptr<SceneMeta> sceneMeta)
    : Asset(std::move(sceneMeta)), m_Path(path), m_Loaded(false)
{
}

SceneAsset::~SceneAsset()
{
    Unload();
}

bool SceneAsset::Load()
{
    if (m_Loaded)
        return true;

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

void SceneAsset::Serialize()
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
}

void SceneAsset::Unload()
{
    m_Data.clear();
    m_Loaded = false;
}

const std::vector<std::byte>& SceneAsset::GetData() const
{
    return m_Data;
}

void SceneAsset::SetData(std::vector<std::byte>&& buffer)
{
    m_Data = std::move(buffer);
}

const std::string& SceneAsset::GetPath() const
{
    return m_Path;
}

bool SceneAsset::IsLoaded() const
{
    return m_Loaded;
}

std::vector<uint8_t> SceneAsset::ReadSceneData(const std::filesystem::path& filePath)
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

    return data;
}
