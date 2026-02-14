#pragma once
#include <memory>
#include <string>
#include "glm/glm.hpp"

#include "Assets/Asset.h"
#include "Assets/AssetUUID.h"



struct MaterialMeta : public AssetMeta
{
    std::unique_ptr<AssetMeta> Clone() const override;
    void Serialize(std::vector<char>& outBuffer, size_t& offset) const override;
    void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override;
};

class MaterialAsset : public Asset
{
public:
    MaterialAsset(const std::string& path, std::unique_ptr<MaterialMeta> meta);
    ~MaterialAsset();

    bool Load();
    void Serialize();

public:
    bool useAlbedoMap = false;
    bool useNormalMap = false;
    bool useMetallicMap = false;
    bool useRoughnessMap = false;
    bool useAOMap = false;
    bool useHeightMap = false;

    glm::vec3 albedoColor;
    float metallic;
    float roughness;
    float ao;
    float normalScale;
    glm::vec2 UVScale;
    glm::vec2 UVOffset;

    AssetUUID albedoTextureUUID;
    AssetUUID normalTextureUUID;
    AssetUUID metallicTextureUUID;
    AssetUUID roughnessTextureUUID;
    AssetUUID aoTextureUUID;
    AssetUUID heightTextureUUID;

private:
    std::string m_Path;
};
