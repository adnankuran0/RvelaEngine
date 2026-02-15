#include "rvelapch.h"
#include "PrefabAsset.h"

#include "Core/Log.h"
#include <fstream>
#include <cstring>

namespace rv {

std::unique_ptr<AssetMeta> PrefabMeta::Clone() const
{
    return std::make_unique<PrefabMeta>(*this);
}

void PrefabMeta::Serialize(std::vector<char>& outBuffer, size_t& offset) const
{
    SerializeBase(outBuffer, offset);

    if (outBuffer.size() < offset + sizeof(entityCount))
        outBuffer.resize(offset + sizeof(entityCount));

    std::memcpy(outBuffer.data() + offset, &entityCount, sizeof(entityCount));
    offset += sizeof(entityCount);
}

void PrefabMeta::Deserialize(const std::vector<char>& inBuffer, size_t& offset)
{
    DeserializeBase(inBuffer, offset);

    std::memcpy(&entityCount, inBuffer.data() + offset, sizeof(entityCount));
    offset += sizeof(entityCount);
}

PrefabAsset::PrefabAsset(const std::string& path,
    std::unique_ptr<PrefabMeta> prefabMeta)
    : Asset(std::move(prefabMeta)), m_Path(path)
{
}

PrefabAsset::~PrefabAsset()
{
    Unload();
}

bool PrefabAsset::Load()
{
    if (m_Loaded)
        return true;

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
    if (!file || header.magic != MAGIC_PREFAB)
    {
        LOG_ERROR("Invalid prefab header");
        return false;
    }

    std::vector<char> metaBuffer(header.metaSize);
    file.read(metaBuffer.data(), header.metaSize);

    size_t metaOffset = 0;
    meta->Deserialize(metaBuffer, metaOffset);

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

    m_Loaded = true;
    return true;
}

void PrefabAsset::Serialize()
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

    AssetHeader header{};
    header.magic = MAGIC_PREFAB;
    header.version = 1;
    header.type = AssetType::Prefab;
    header.metaSize = static_cast<uint32_t>(metaBuffer.size());
    header.dataOffset = sizeof(AssetHeader) + header.metaSize;

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
}

void PrefabAsset::Unload()
{
    m_Data.clear();
    m_Loaded = false;
}

const std::vector<std::byte>& PrefabAsset::GetData() const
{
    return m_Data;
}

void PrefabAsset::SetData(std::vector<std::byte>&& buffer)
{
    m_Data = std::move(buffer);
}

const std::string& PrefabAsset::GetPath() const
{
    return m_Path;
}

bool PrefabAsset::IsLoaded() const
{
    return m_Loaded;
}


}