#pragma once
#include "Asset/Asset.h"
#include "Asset/AssetUUID.h"
#include "Renderer/Sampler.h"
#include <glm/glm.hpp>
#include "Renderer/RenderTypes.h"

namespace rv {

class MaterialAsset : public Asset
{
public:
    explicit MaterialAsset(AssetUUID uuid) : Asset(uuid) {}

    AssetUUID albedoTextureUUID;
    AssetUUID normalTextureUUID;
    AssetUUID metallicTextureUUID;
    AssetUUID roughnessTextureUUID;
    AssetUUID aoTextureUUID;
    AssetUUID heightTextureUUID;

    ShadingMode shadingMode = ShadingMode::Lit;
    bool receiveShadows = true;

    TransparencyMode transparencyMode = TransparencyMode::Opaque;
    BlendMode blendMode = BlendMode::Mix;
    CullMode cullMode = CullMode::Back;
    float alphaCutoff = 0.5f;

    glm::vec4 albedoColor = glm::vec4(1.0f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float emissiveIntensity = 0.0f;
    float metallic = 0.0f;
    float specular = 1.0f;
    float roughness = 1.0f;
    float ao = 1.0f;
    float normalScale = 1.0f;
    float heightScale = 0.1f;
    glm::vec2 UVScale = glm::vec2(1.0f);
    glm::vec2 UVOffset = glm::vec2(0.0f);

    bool useAlbedoMap = false;
    bool useNormalMap = false;
    bool useMetallicMap = false;
    bool useRoughnessMap = false;
    bool useAOMap = false;
    bool useHeightMap = false;

    SamplerDesc samplerDesc;

    bool IsValid() const { return true; }

private:
    friend class MaterialLoader;
};

}