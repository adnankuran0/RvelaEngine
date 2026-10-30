#pragma once
#include "Asset/Asset.h"
#include "Asset/AssetUUID.h"
#include "Renderer/Sampler.h"
#include <glm/glm.hpp>
#include "Renderer/RenderTypes.h"
#include "Renderer/CustomShaderGenerator.h"
#include <unordered_map>

namespace rv {

class MaterialAsset : public Asset
{
public:
    explicit MaterialAsset(AssetUUID uuid) : Asset(uuid) {}

    MaterialAsset& operator=(const MaterialAsset& other)
    {
        if (this == &other) return *this;
        customShaderUUID = other.customShaderUUID;
        customUniforms = other.customUniforms;
        albedoTextureUUID = other.albedoTextureUUID;
        normalTextureUUID = other.normalTextureUUID;
        metallicTextureUUID = other.metallicTextureUUID;
        roughnessTextureUUID = other.roughnessTextureUUID;
        aoTextureUUID = other.aoTextureUUID;
        heightTextureUUID = other.heightTextureUUID;
        armTextureUUID = other.armTextureUUID;
        shadingMode = other.shadingMode;
        receiveShadows = other.receiveShadows;
        transparencyMode = other.transparencyMode;
        blendMode = other.blendMode;
        cullMode = other.cullMode;
        alphaCutoff = other.alphaCutoff;
        billboardMode = other.billboardMode;
        albedoColor = other.albedoColor;
        emissiveColor = other.emissiveColor;
        emissiveIntensity = other.emissiveIntensity;
        metallic = other.metallic;
        specular = other.specular;
        roughness = other.roughness;
        ao = other.ao;
        normalScale = other.normalScale;
        heightScale = other.heightScale;
        invertHeight = other.invertHeight;
        UVScale = other.UVScale;
        UVOffset = other.UVOffset;
        useTriplanar = other.useTriplanar;
        useWorldTriplanar = other.useWorldTriplanar;
        triplanarSharpness = other.triplanarSharpness;
        useAlbedoMap = other.useAlbedoMap;
        useNormalMap = other.useNormalMap;
        useMetallicMap = other.useMetallicMap;
        useRoughnessMap = other.useRoughnessMap;
        useAOMap = other.useAOMap;
        useHeightMap = other.useHeightMap;
        useARMMap = other.useARMMap;
        samplerDesc = other.samplerDesc;
        hasOutline = other.hasOutline;
        outlineColor = other.outlineColor;
        outlineWidth = other.outlineWidth;
        outlineFixedSize = other.outlineFixedSize;
        outlineXRay = other.outlineXRay;
        return *this;
    }

    AssetUUID customShaderUUID = AssetUUID::Invalid();
    std::unordered_map<std::string, CustomUniformValue> customUniforms;

    AssetUUID albedoTextureUUID;
    AssetUUID normalTextureUUID;
    AssetUUID metallicTextureUUID;
    AssetUUID roughnessTextureUUID;
    AssetUUID aoTextureUUID;
    AssetUUID heightTextureUUID;
    AssetUUID armTextureUUID;

    ShadingMode shadingMode = ShadingMode::Lit;
    bool receiveShadows = true;

    TransparencyMode transparencyMode = TransparencyMode::Opaque;
    BlendMode blendMode = BlendMode::Mix;
    CullMode cullMode = CullMode::Back;
    float alphaCutoff = 0.5f;

    BillboardMode billboardMode = BillboardMode::Disabled;

    glm::vec4 albedoColor = glm::vec4(1.0f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    float emissiveIntensity = 0.0f;
    float metallic = 0.0f;
    float specular = 1.0f;
    float roughness = 1.0f;
    float ao = 1.0f;
    float normalScale = 1.0f;
    float heightScale = 0.05f;
    bool invertHeight = false;
    glm::vec2 UVScale = glm::vec2(1.0f);
    glm::vec2 UVOffset = glm::vec2(0.0f);
    bool useTriplanar = false;
    bool useWorldTriplanar = false;
    float triplanarSharpness = 4.0f;

    bool useAlbedoMap = false;
    bool useNormalMap = false;
    bool useMetallicMap = false;
    bool useRoughnessMap = false;
    bool useAOMap = false;
    bool useHeightMap = false;
    bool useARMMap = false;

    SamplerDesc samplerDesc;

    bool hasOutline = false;
    glm::vec4 outlineColor = glm::vec4(1.0f, 0.7f, 0.1f, 1.0f);
    float outlineWidth = 2.0f;
    bool outlineFixedSize = true;
    bool outlineXRay = false;

    bool IsValid() const { return true; }

private:
    friend class MaterialLoader;
};

}
