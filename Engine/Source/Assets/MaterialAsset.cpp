#include "rvelapch.h"
#include "MaterialAsset.h"

#include "AssetMeta.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <glm/glm.hpp>
#include "Core/Log.h"

namespace rv {

std::unique_ptr<AssetMeta> MaterialMeta::Clone() const
{
    return std::make_unique<MaterialMeta>(*this);
}

void MaterialMeta::Serialize(std::vector<char>& outBuffer, size_t& offset) const
{
    SerializeBase(outBuffer, offset);
}

void MaterialMeta::Deserialize(const std::vector<char>& inBuffer, size_t& offset)
{
    DeserializeBase(inBuffer, offset);
}

MaterialAsset::MaterialAsset(const std::string& path, std::unique_ptr<MaterialMeta> meta)
    : Asset(std::move(meta)), m_Path(path),
    albedoColor(1.0f),
    metallic(0.0f),
    roughness(1.0f),
    ao(1.0f),
    normalScale(1.0f),
    UVScale(1.0f),
    UVOffset(0.0f)
{
}

MaterialAsset::~MaterialAsset() = default;

bool MaterialAsset::Load()
{
    std::ifstream file(m_Path, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Failed to open .rmat: {}", m_Path);
        return false;
    }

    AssetHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.magic != MAGIC_MATERIAL)
    {
        LOG_ERROR("Invalid material magic!");
        return false;
    }

    std::vector<char> metaBuffer(header.metaSize);
    file.read(metaBuffer.data(), header.metaSize);

    size_t offset = 0;
    GetMetaAs<MaterialMeta>()->Deserialize(metaBuffer, offset);

    std::vector<char> dataBuffer((std::istreambuf_iterator<char>(file)), {});
    offset = 0;

    auto read = [&](void* dst, size_t size)
        {
            std::memcpy(dst, dataBuffer.data() + offset, size);
            offset += size;
        };

    read(&albedoColor, sizeof(glm::vec3));
    read(&metallic, sizeof(float));
    read(&roughness, sizeof(float));
    read(&ao, sizeof(float));
    read(&normalScale, sizeof(float));
    read(&UVScale, sizeof(glm::vec2));
    read(&UVOffset, sizeof(glm::vec2));

    read(&albedoTextureUUID, sizeof(AssetUUID));
    read(&normalTextureUUID, sizeof(AssetUUID));
    read(&metallicTextureUUID, sizeof(AssetUUID));
    read(&roughnessTextureUUID, sizeof(AssetUUID));
    read(&aoTextureUUID, sizeof(AssetUUID));
    read(&heightTextureUUID, sizeof(AssetUUID));

    read(&useAlbedoMap, sizeof(bool));
    read(&useNormalMap, sizeof(bool));
    read(&useMetallicMap, sizeof(bool));
    read(&useRoughnessMap, sizeof(bool));
    read(&useAOMap, sizeof(bool));
    read(&useHeightMap, sizeof(bool));

    return true;
}

void MaterialAsset::Serialize()
{
    std::vector<char> metaBuffer;
    size_t metaOffset = 0;
    GetMetaAs<MaterialMeta>()->Serialize(metaBuffer, metaOffset);

    std::vector<char> dataBuffer;
    size_t dataOffset = 0;

    auto write = [&](const void* data, size_t size)
        {
            if (dataBuffer.size() < dataOffset + size)
                dataBuffer.resize(dataOffset + size);
            std::memcpy(dataBuffer.data() + dataOffset, data, size);
            dataOffset += size;
        };

    write(&albedoColor, sizeof(glm::vec3));
    write(&metallic, sizeof(float));
    write(&roughness, sizeof(float));
    write(&ao, sizeof(float));
    write(&normalScale, sizeof(float));
    write(&UVScale, sizeof(glm::vec2));
    write(&UVOffset, sizeof(glm::vec2));

    write(&albedoTextureUUID, sizeof(AssetUUID));
    write(&normalTextureUUID, sizeof(AssetUUID));
    write(&metallicTextureUUID, sizeof(AssetUUID));
    write(&roughnessTextureUUID, sizeof(AssetUUID));
    write(&aoTextureUUID, sizeof(AssetUUID));
    write(&heightTextureUUID, sizeof(AssetUUID));

    write(&useAlbedoMap, sizeof(bool));
    write(&useNormalMap, sizeof(bool));
    write(&useMetallicMap, sizeof(bool));
    write(&useRoughnessMap, sizeof(bool));
    write(&useAOMap, sizeof(bool));
    write(&useHeightMap, sizeof(bool));

    AssetHeader header{};
    header.magic = MAGIC_MATERIAL;
    header.version = 1;
    header.type = AssetType::Material;
    header.metaSize = static_cast<uint32_t>(metaBuffer.size());
    header.dataOffset = sizeof(AssetHeader) + header.metaSize;

    std::ofstream out(m_Path, std::ios::binary);
    if (!out)
    {
        LOG_ERROR("Failed to write .rmat file: {}", m_Path);
        return;
    }

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    out.write(metaBuffer.data(), metaBuffer.size());
    out.write(dataBuffer.data(), dataBuffer.size());
}

}