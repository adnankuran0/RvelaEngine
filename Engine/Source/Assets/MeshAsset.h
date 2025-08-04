#pragma once
#include "Assets/AssetMeta.h"
#include "Assets/Asset.h"

class MeshMeta : public AssetMeta
{
public:
    std::unique_ptr<AssetMeta> Clone() const override
    {
        return std::make_unique<MeshMeta>(*this);
    }

    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override
    {
        SerializeBase(outBuffer, offset); // UUID, sourcePath, type
    }

    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override
    {
        DeserializeBase(inBuffer,offset); // UUID, sourcePath, type
    }

};

class MeshAsset : public Asset
{
public:
    std::string meshName;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    uint32_t indexCount;
    AssetUUID materialUUID;
};




#if 0

struct MeshComponent
{
    Ref<MeshAsset> mesh;
};

struct MeshRendererComponent
{

};

#endif