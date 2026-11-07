#pragma once
#include "Asset/AssetUUID.h"
#include "Asset/Types/MaterialAsset.h"
#include "Asset/Types/TextureAsset.h"
#include "Asset/Types/ShaderAsset.h"
#include "Core/Ref.h"
#include "Renderer/Sampler.h"
#include <glm/glm.hpp>
#include <bitset>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "Asset/AssetManager.h"

namespace rv {

enum class MatField : uint8_t {
    AlbedoColor = 0,
    EmissiveColor,
    EmissiveIntensity,
    Metallic,
    Specular,
    Roughness,
    AO,
    NormalScale,
    HeightScale,
    InvertHeight,
    UVScale,
    UVOffset,
    AlbedoTex,
    NormalTex,
    MetallicTex,
    RoughnessTex,
    AOTex,
    HeightTex,
    ARMTex,
    UseARMMap,
    TransparencyMode,
    BlendMode,
    CullMode,
    AlphaCutoff,
    ShadingMode,
    ReceiveShadows,
    BillboardMode,
    UseTriplanar,
    UseWorldTriplanar,
    TriplanarSharpness,
    CustomShader,
    TextureFilterMode,
    TextureWrapMode,
    TextureAnisotropyMode,
    HasOutline,
    OutlineColor,
    OutlineWidth,
    OutlineFixedSize,
    OutlineXRay,
    COUNT
};

struct MaterialOverrideData
{
    std::bitset<(size_t)MatField::COUNT> overrideMask;

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

    bool hasOutline = false;
    glm::vec4 outlineColor = glm::vec4(1.0f, 0.7f, 0.1f, 1.0f);
    float outlineWidth = 2.0f;
    bool outlineFixedSize = true;
    bool outlineXRay = false;

    TransparencyMode transparencyMode = TransparencyMode::Opaque;
    BlendMode blendMode = BlendMode::Mix;
    CullMode cullMode = CullMode::Back;
    float alphaCutoff = 0.5f;

    BillboardMode billboardMode = BillboardMode::Disabled;

    ShadingMode shadingMode = ShadingMode::Lit;
    bool receiveShadows = true;

    bool useAlbedoMap = false;
    bool useNormalMap = false;
    bool useMetallicMap = false;
    bool useRoughnessMap = false;
    bool useAOMap = false;
    bool useHeightMap = false;
    bool useARMMap = false;

    Ref<TextureAsset> albedoTex;
    Ref<TextureAsset> normalTex;
    Ref<TextureAsset> metallicTex;
    Ref<TextureAsset> roughnessTex;
    Ref<TextureAsset> aoTex;
    Ref<TextureAsset> heightTex;
    Ref<TextureAsset> armTex;

    AssetUUID customShaderUUID = AssetUUID::Invalid();
    Ref<ShaderAsset> customShader;
    std::unique_ptr<std::unordered_map<std::string, CustomUniformValue>> customUniformOverrides;

    TextureFilter textureFilter = TextureFilter::Default;
    TextureWrap textureWrap = TextureWrap::Default;
    TextureAnisotropy textureAnisotropy = TextureAnisotropy::Default;

    MaterialOverrideData() = default;
    MaterialOverrideData(const MaterialOverrideData& other)
    {
        *this = other;
    }
    MaterialOverrideData& operator=(const MaterialOverrideData& other)
    {
        if (this == &other) return *this;
        overrideMask = other.overrideMask;
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
        hasOutline = other.hasOutline;
        outlineColor = other.outlineColor;
        outlineWidth = other.outlineWidth;
        outlineFixedSize = other.outlineFixedSize;
        outlineXRay = other.outlineXRay;
        transparencyMode = other.transparencyMode;
        blendMode = other.blendMode;
        cullMode = other.cullMode;
        alphaCutoff = other.alphaCutoff;
        billboardMode = other.billboardMode;
        shadingMode = other.shadingMode;
        receiveShadows = other.receiveShadows;
        useAlbedoMap = other.useAlbedoMap;
        useNormalMap = other.useNormalMap;
        useMetallicMap = other.useMetallicMap;
        useRoughnessMap = other.useRoughnessMap;
        useAOMap = other.useAOMap;
        useHeightMap = other.useHeightMap;
        useARMMap = other.useARMMap;
        albedoTex = other.albedoTex;
        normalTex = other.normalTex;
        metallicTex = other.metallicTex;
        roughnessTex = other.roughnessTex;
        aoTex = other.aoTex;
        heightTex = other.heightTex;
        armTex = other.armTex;
        customShaderUUID = other.customShaderUUID;
        customShader = other.customShader;
        textureFilter = other.textureFilter;
        textureWrap = other.textureWrap;
        textureAnisotropy = other.textureAnisotropy;
        if (other.customUniformOverrides)
            customUniformOverrides = std::make_unique<std::unordered_map<std::string, CustomUniformValue>>(*other.customUniformOverrides);
        else
            customUniformOverrides.reset();
        return *this;
    }
};

class MaterialInstance
{
public:
    MaterialInstance() = default;
    ~MaterialInstance() = default;

    MaterialInstance(const MaterialInstance& other)
        : m_SourceAsset(other.m_SourceAsset)
        , m_Sampler(other.m_Sampler)
    {
        if (other.m_Overrides)
            m_Overrides = std::make_unique<MaterialOverrideData>(*other.m_Overrides);
    }

    MaterialInstance& operator=(const MaterialInstance& other)
    {
        if (this == &other) return *this;
        m_SourceAsset = other.m_SourceAsset;
        m_Sampler = other.m_Sampler;
        if (other.m_Overrides)
            m_Overrides = std::make_unique<MaterialOverrideData>(*other.m_Overrides);
        else
            m_Overrides.reset();
        return *this;
    }

    MaterialInstance(MaterialInstance&& other) noexcept = default;
    MaterialInstance& operator=(MaterialInstance&& other) noexcept = default;

    static MaterialInstance CreateFromAsset(const Ref<MaterialAsset>& asset);

    Ref<MaterialAsset> GetSourceAsset() const { return m_SourceAsset; }
    AssetUUID GetSourceUUID() const { return m_SourceAsset ? m_SourceAsset->GetUUID() : AssetUUID::Invalid(); }
    bool IsDefault() const { return m_SourceAsset.Get() == nullptr; }

    Ref<TextureAsset> GetAlbedoTexture() const { return IsOverridden(MatField::AlbedoTex) ? m_Overrides->albedoTex : (m_SourceAsset ? m_SourceAsset->GetAlbedoTexture() : nullptr); }
    Ref<TextureAsset> GetNormalTexture() const { return IsOverridden(MatField::NormalTex) ? m_Overrides->normalTex : (m_SourceAsset ? m_SourceAsset->GetNormalTexture() : nullptr); }
    Ref<TextureAsset> GetMetallicTexture() const { return IsOverridden(MatField::MetallicTex) ? m_Overrides->metallicTex : (m_SourceAsset ? m_SourceAsset->GetMetallicTexture() : nullptr); }
    Ref<TextureAsset> GetRoughnessTex() const { return IsOverridden(MatField::RoughnessTex) ? m_Overrides->roughnessTex : (m_SourceAsset ? m_SourceAsset->GetRoughnessTexture() : nullptr); }
    Ref<TextureAsset> GetAOTexture() const { return IsOverridden(MatField::AOTex) ? m_Overrides->aoTex : (m_SourceAsset ? m_SourceAsset->GetAOTexture() : nullptr); }
    Ref<TextureAsset> GetHeightTexture() const { return IsOverridden(MatField::HeightTex) ? m_Overrides->heightTex : (m_SourceAsset ? m_SourceAsset->GetHeightTexture() : nullptr); }
    Ref<TextureAsset> GetARMTexture() const { return IsOverridden(MatField::ARMTex) ? m_Overrides->armTex : (m_SourceAsset ? m_SourceAsset->GetARMTexture() : nullptr); }

    void SetAlbedoTexture(Ref<TextureAsset> tex) { auto& o = EnsureOverrides(); o.albedoTex = tex; o.useAlbedoMap = static_cast<bool>(tex); SetOverride(MatField::AlbedoTex); }
    void SetNormalTexture(Ref<TextureAsset> tex) { auto& o = EnsureOverrides(); o.normalTex = tex; o.useNormalMap = static_cast<bool>(tex); SetOverride(MatField::NormalTex); }
    void SetMetallicTexture(Ref<TextureAsset> tex) { auto& o = EnsureOverrides(); o.metallicTex = tex; o.useMetallicMap = static_cast<bool>(tex); SetOverride(MatField::MetallicTex); }
    void SetRoughnessTexture(Ref<TextureAsset> tex) { auto& o = EnsureOverrides(); o.roughnessTex = tex; o.useRoughnessMap = static_cast<bool>(tex); SetOverride(MatField::RoughnessTex); }
    void SetAOTexture(Ref<TextureAsset> tex) { auto& o = EnsureOverrides(); o.aoTex = tex; o.useAOMap = static_cast<bool>(tex); SetOverride(MatField::AOTex); }
    void SetHeightTexture(Ref<TextureAsset> tex) { auto& o = EnsureOverrides(); o.heightTex = tex; o.useHeightMap = static_cast<bool>(tex); SetOverride(MatField::HeightTex); }
    void SetARMTexture(Ref<TextureAsset> tex) { auto& o = EnsureOverrides(); o.armTex = tex; o.useARMMap = static_cast<bool>(tex); SetOverride(MatField::ARMTex); }

    void ClearAlbedoTexture() { if (m_Overrides) { m_Overrides->albedoTex = nullptr; m_Overrides->useAlbedoMap = false; } ClearOverride(MatField::AlbedoTex); }
    void ClearNormalTexture() { if (m_Overrides) { m_Overrides->normalTex = nullptr; m_Overrides->useNormalMap = false; } ClearOverride(MatField::NormalTex); }
    void ClearMetallicTexture() { if (m_Overrides) { m_Overrides->metallicTex = nullptr; m_Overrides->useMetallicMap = false; } ClearOverride(MatField::MetallicTex); }
    void ClearRoughnessTexture() { if (m_Overrides) { m_Overrides->roughnessTex = nullptr; m_Overrides->useRoughnessMap = false; } ClearOverride(MatField::RoughnessTex); }
    void ClearAOTexture() { if (m_Overrides) { m_Overrides->aoTex = nullptr; m_Overrides->useAOMap = false; } ClearOverride(MatField::AOTex); }
    void ClearHeightTexture() { if (m_Overrides) { m_Overrides->heightTex = nullptr; m_Overrides->useHeightMap = false; } ClearOverride(MatField::HeightTex); }
    void ClearARMTexture() { if (m_Overrides) { m_Overrides->armTex = nullptr; m_Overrides->useARMMap = false; } ClearOverride(MatField::ARMTex); }

    void SetAlbedoColor(const glm::vec4& v) { EnsureOverrides().albedoColor = v; SetOverride(MatField::AlbedoColor); }
    void SetEmissiveColor(const glm::vec3& v) { EnsureOverrides().emissiveColor = v; SetOverride(MatField::EmissiveColor); }
    void SetEmissiveIntensity(float v) { EnsureOverrides().emissiveIntensity = v; SetOverride(MatField::EmissiveIntensity); }
    void SetMetallic(float v) { EnsureOverrides().metallic = v; SetOverride(MatField::Metallic); }
    void SetSpecular(float v) { EnsureOverrides().specular = v; SetOverride(MatField::Specular); }
    void SetRoughness(float v) { EnsureOverrides().roughness = v; SetOverride(MatField::Roughness); }
    void SetAO(float v) { EnsureOverrides().ao = v; SetOverride(MatField::AO); }
    void SetNormalScale(float v) { EnsureOverrides().normalScale = v; SetOverride(MatField::NormalScale); }
    void SetHeightScale(float v) { EnsureOverrides().heightScale = v; SetOverride(MatField::HeightScale); }
    void SetInvertHeight(bool v) { EnsureOverrides().invertHeight = v; SetOverride(MatField::InvertHeight); }
    void SetUVScale(const glm::vec2& v) { EnsureOverrides().UVScale = v; SetOverride(MatField::UVScale); }
    void SetUVOffset(const glm::vec2& v) { EnsureOverrides().UVOffset = v; SetOverride(MatField::UVOffset); }

    void SetTransparencyMode(TransparencyMode mode) { EnsureOverrides().transparencyMode = mode; SetOverride(MatField::TransparencyMode); }
    void SetBlendMode(BlendMode mode) { EnsureOverrides().blendMode = mode; SetOverride(MatField::BlendMode); }
    void SetCullMode(CullMode mode) { EnsureOverrides().cullMode = mode; SetOverride(MatField::CullMode); }
    void SetAlphaCutoff(float v) { EnsureOverrides().alphaCutoff = v; SetOverride(MatField::AlphaCutoff); }

    void SetBillboardMode(BillboardMode mode) { EnsureOverrides().billboardMode = mode; SetOverride(MatField::BillboardMode); }

    void SetShadingMode(ShadingMode mode) { EnsureOverrides().shadingMode = mode; SetOverride(MatField::ShadingMode); }
    void SetReceiveShadows(bool v) { EnsureOverrides().receiveShadows = v; SetOverride(MatField::ReceiveShadows); }
    void SetUseTriplanar(bool v) { EnsureOverrides().useTriplanar = v; SetOverride(MatField::UseTriplanar); }
    void SetUseWorldTriplanar(bool v) { EnsureOverrides().useWorldTriplanar = v; SetOverride(MatField::UseWorldTriplanar); }
    void SetTriplanarSharpness(float v) { EnsureOverrides().triplanarSharpness = v; SetOverride(MatField::TriplanarSharpness); }

    void SetHasOutline(bool v) { EnsureOverrides().hasOutline = v; SetOverride(MatField::HasOutline); }
    void SetOutlineColor(const glm::vec4& v) { EnsureOverrides().outlineColor = v; SetOverride(MatField::OutlineColor); }
    void SetOutlineWidth(float v) { EnsureOverrides().outlineWidth = v; SetOverride(MatField::OutlineWidth); }
    void SetOutlineFixedSize(bool v) { EnsureOverrides().outlineFixedSize = v; SetOverride(MatField::OutlineFixedSize); }
    void SetOutlineXRay(bool v) { EnsureOverrides().outlineXRay = v; SetOverride(MatField::OutlineXRay); }

    glm::vec4 GetAlbedoColor() const { return IsOverridden(MatField::AlbedoColor) ? m_Overrides->albedoColor : (m_SourceAsset ? m_SourceAsset->albedoColor : glm::vec4(1.0f)); }
    glm::vec3 GetEmissiveColor() const { return IsOverridden(MatField::EmissiveColor) ? m_Overrides->emissiveColor : (m_SourceAsset ? m_SourceAsset->emissiveColor : glm::vec3(0.0f)); }
    float GetEmissiveIntensity() const { return IsOverridden(MatField::EmissiveIntensity) ? m_Overrides->emissiveIntensity : (m_SourceAsset ? m_SourceAsset->emissiveIntensity : 0.0f); }
    float GetMetallic() const { return IsOverridden(MatField::Metallic) ? m_Overrides->metallic : (m_SourceAsset ? m_SourceAsset->metallic : 0.0f); }
    float GetSpecular() const { return IsOverridden(MatField::Specular) ? m_Overrides->specular : (m_SourceAsset ? m_SourceAsset->specular : 1.0f); }
    float GetRoughness() const { return IsOverridden(MatField::Roughness) ? m_Overrides->roughness : (m_SourceAsset ? m_SourceAsset->roughness : 1.0f); }
    float GetAO() const { return IsOverridden(MatField::AO) ? m_Overrides->ao : (m_SourceAsset ? m_SourceAsset->ao : 1.0f); }
    float GetNormalScale() const { return IsOverridden(MatField::NormalScale) ? m_Overrides->normalScale : (m_SourceAsset ? m_SourceAsset->normalScale : 1.0f); }
    float GetHeightScale() const { return IsOverridden(MatField::HeightScale) ? m_Overrides->heightScale : (m_SourceAsset ? m_SourceAsset->heightScale : 0.05f); }
    bool GetInvertHeight() const { return IsOverridden(MatField::InvertHeight) ? m_Overrides->invertHeight : (m_SourceAsset ? m_SourceAsset->invertHeight : false); }
    glm::vec2 GetUVScale() const { return IsOverridden(MatField::UVScale) ? m_Overrides->UVScale : (m_SourceAsset ? m_SourceAsset->UVScale : glm::vec2(1.0f)); }
    glm::vec2 GetUVOffset() const { return IsOverridden(MatField::UVOffset) ? m_Overrides->UVOffset : (m_SourceAsset ? m_SourceAsset->UVOffset : glm::vec2(0.0f)); }

    TransparencyMode GetTransparencyMode() const { return IsOverridden(MatField::TransparencyMode) ? m_Overrides->transparencyMode : (m_SourceAsset ? m_SourceAsset->transparencyMode : TransparencyMode::Opaque); }
    BlendMode GetBlendMode() const { return IsOverridden(MatField::BlendMode) ? m_Overrides->blendMode : (m_SourceAsset ? m_SourceAsset->blendMode : BlendMode::Mix); }
    CullMode GetCullMode() const { return IsOverridden(MatField::CullMode) ? m_Overrides->cullMode : (m_SourceAsset ? m_SourceAsset->cullMode : CullMode::Back); }
    float GetAlphaCutoff() const { return IsOverridden(MatField::AlphaCutoff) ? m_Overrides->alphaCutoff : (m_SourceAsset ? m_SourceAsset->alphaCutoff : 0.5f); }

    BillboardMode GetBillboardMode() const { return IsOverridden(MatField::BillboardMode) ? m_Overrides->billboardMode : (m_SourceAsset ? m_SourceAsset->billboardMode : BillboardMode::Disabled); }

    ShadingMode GetShadingMode() const { return IsOverridden(MatField::ShadingMode) ? m_Overrides->shadingMode : (m_SourceAsset ? m_SourceAsset->shadingMode : ShadingMode::Lit); }
    bool GetReceiveShadows() const { return IsOverridden(MatField::ReceiveShadows) ? m_Overrides->receiveShadows : (m_SourceAsset ? m_SourceAsset->receiveShadows : true); }
    bool GetUseTriplanar() const { return IsOverridden(MatField::UseTriplanar) ? m_Overrides->useTriplanar : (m_SourceAsset ? m_SourceAsset->useTriplanar : false); }
    bool GetUseWorldTriplanar() const { return IsOverridden(MatField::UseWorldTriplanar) ? m_Overrides->useWorldTriplanar : (m_SourceAsset ? m_SourceAsset->useWorldTriplanar : false); }
    float GetTriplanarSharpness() const { return IsOverridden(MatField::TriplanarSharpness) ? m_Overrides->triplanarSharpness : (m_SourceAsset ? m_SourceAsset->triplanarSharpness : 4.0f); }

    bool GetHasOutline() const { return IsOverridden(MatField::HasOutline) ? m_Overrides->hasOutline : (m_SourceAsset ? m_SourceAsset->hasOutline : false); }
    glm::vec4 GetOutlineColor() const { return IsOverridden(MatField::OutlineColor) ? m_Overrides->outlineColor : (m_SourceAsset ? m_SourceAsset->outlineColor : glm::vec4(1.0f, 0.7f, 0.1f, 1.0f)); }
    float GetOutlineWidth() const { return IsOverridden(MatField::OutlineWidth) ? m_Overrides->outlineWidth : (m_SourceAsset ? m_SourceAsset->outlineWidth : 2.0f); }
    bool GetOutlineFixedSize() const { return IsOverridden(MatField::OutlineFixedSize) ? m_Overrides->outlineFixedSize : (m_SourceAsset ? m_SourceAsset->outlineFixedSize : true); }
    bool GetOutlineXRay() const { return IsOverridden(MatField::OutlineXRay) ? m_Overrides->outlineXRay : (m_SourceAsset ? m_SourceAsset->outlineXRay : false); }

    bool UsesAlbedoMap() const { return IsOverridden(MatField::AlbedoTex) ? m_Overrides->useAlbedoMap : (m_SourceAsset ? m_SourceAsset->useAlbedoMap : false); }
    bool UsesNormalMap() const { return IsOverridden(MatField::NormalTex) ? m_Overrides->useNormalMap : (m_SourceAsset ? m_SourceAsset->useNormalMap : false); }
    bool UsesMetallicMap() const { return IsOverridden(MatField::MetallicTex) ? m_Overrides->useMetallicMap : (m_SourceAsset ? m_SourceAsset->useMetallicMap : false); }
    bool UsesRoughnessMap() const { return IsOverridden(MatField::RoughnessTex) ? m_Overrides->useRoughnessMap : (m_SourceAsset ? m_SourceAsset->useRoughnessMap : false); }
    bool UsesAOMap() const { return IsOverridden(MatField::AOTex) ? m_Overrides->useAOMap : (m_SourceAsset ? m_SourceAsset->useAOMap : false); }
    bool UsesHeightMap() const { return IsOverridden(MatField::HeightTex) ? m_Overrides->useHeightMap : (m_SourceAsset ? m_SourceAsset->useHeightMap : false); }

    void ClearAlbedoColor() { ClearOverride(MatField::AlbedoColor); }
    void ClearEmissiveColor() { ClearOverride(MatField::EmissiveColor); }
    void ClearEmissiveIntensity() { ClearOverride(MatField::EmissiveIntensity); }
    void ClearMetallic() { ClearOverride(MatField::Metallic); }
    void ClearSpecular() { ClearOverride(MatField::Specular); }
    void ClearRoughness() { ClearOverride(MatField::Roughness); }
    void ClearAO() { ClearOverride(MatField::AO); }
    void ClearNormalScale() { ClearOverride(MatField::NormalScale); }
    void ClearHeightScale() { ClearOverride(MatField::HeightScale); }
    void ClearInvertHeight() { ClearOverride(MatField::InvertHeight); }
    void ClearUVScale() { ClearOverride(MatField::UVScale); }
    void ClearUVOffset() { ClearOverride(MatField::UVOffset); }

    bool GetUseARMMap() const { return IsOverridden(MatField::UseARMMap) ? m_Overrides->useARMMap : (m_SourceAsset ? m_SourceAsset->useARMMap : false); }
    void SetUseARMMap(bool v) { EnsureOverrides().useARMMap = v; SetOverride(MatField::UseARMMap); }
    void ClearUseARMMap() { ClearOverride(MatField::UseARMMap); }
    bool UsesARMMap() const { return GetUseARMMap() && static_cast<bool>(GetARMTexture()); }

    void ClearTransparencyMode() { ClearOverride(MatField::TransparencyMode); }
    void ClearBlendMode() { ClearOverride(MatField::BlendMode); }
    void ClearCullMode() { ClearOverride(MatField::CullMode); }
    void ClearAlphaCutoff() { ClearOverride(MatField::AlphaCutoff); }

    void ClearBillboardMode() { ClearOverride(MatField::BillboardMode); }

    void ClearShadingMode() { ClearOverride(MatField::ShadingMode); }
    void ClearReceiveShadows() { ClearOverride(MatField::ReceiveShadows); }
    void ClearUseTriplanar() { ClearOverride(MatField::UseTriplanar); }
    void ClearUseWorldTriplanar() { ClearOverride(MatField::UseWorldTriplanar); }
    void ClearTriplanarSharpness() { ClearOverride(MatField::TriplanarSharpness); }

    void ClearHasOutline() { ClearOverride(MatField::HasOutline); }
    void ClearOutlineColor() { ClearOverride(MatField::OutlineColor); }
    void ClearOutlineWidth() { ClearOverride(MatField::OutlineWidth); }
    void ClearOutlineFixedSize() { ClearOverride(MatField::OutlineFixedSize); }
    void ClearOutlineXRay() { ClearOverride(MatField::OutlineXRay); }

    AssetUUID GetCustomShaderUUID() const {
        return IsOverridden(MatField::CustomShader) ? (m_Overrides ? m_Overrides->customShaderUUID : AssetUUID::Invalid()) : (m_SourceAsset ? m_SourceAsset->customShaderUUID : AssetUUID::Invalid());
    }
    void SetCustomShader(const AssetUUID& uuid);
    void ClearCustomShader();
    Ref<ShaderAsset> GetCustomShaderAsset() const {
        return IsOverridden(MatField::CustomShader) ? (m_Overrides ? m_Overrides->customShader : nullptr) : (m_SourceAsset ? m_SourceAsset->GetCustomShaderAsset() : nullptr);
    }

    CustomUniformValue GetCustomUniform(const std::string& name) const;
    void SetCustomUniform(const std::string& name, const CustomUniformValue& val);
    void ClearCustomUniform(const std::string& name);
    bool IsCustomUniformOverridden(const std::string& name) const;
    void ClearAllCustomUniformOverrides();
    const std::unordered_map<std::string, CustomUniformValue>& GetCustomUniformOverrides() const {
        static const std::unordered_map<std::string, CustomUniformValue> s_Empty;
        return (m_Overrides && m_Overrides->customUniformOverrides) ? *m_Overrides->customUniformOverrides : s_Empty;
    }
    const std::unordered_map<std::string, CustomUniformValue>& GetCachedCustomUniforms() const {
        static const std::unordered_map<std::string, CustomUniformValue> s_Empty;
        return m_SourceAsset ? m_SourceAsset->customUniforms : s_Empty;
    }

    Sampler& GetSampler() { return m_Sampler; }
    const Sampler& GetSampler() const { return m_Sampler; }

    TextureFilter GetTextureFilter() const { return IsOverridden(MatField::TextureFilterMode) ? m_Overrides->textureFilter : (m_SourceAsset ? m_SourceAsset->samplerDesc.filter : TextureFilter::Default); }
    void SetTextureFilter(TextureFilter f) { EnsureOverrides().textureFilter = f; SetOverride(MatField::TextureFilterMode); m_Sampler.GetDesc().filter = f; }
    void ClearTextureFilter() { ClearOverride(MatField::TextureFilterMode); m_Sampler.GetDesc().filter = m_SourceAsset ? m_SourceAsset->samplerDesc.filter : TextureFilter::Default; }

    TextureWrap GetTextureWrap() const { return IsOverridden(MatField::TextureWrapMode) ? m_Overrides->textureWrap : (m_SourceAsset ? m_SourceAsset->samplerDesc.repeat : TextureWrap::Default); }
    void SetTextureWrap(TextureWrap w) { EnsureOverrides().textureWrap = w; SetOverride(MatField::TextureWrapMode); m_Sampler.GetDesc().repeat = w; }
    void ClearTextureWrap() { ClearOverride(MatField::TextureWrapMode); m_Sampler.GetDesc().repeat = m_SourceAsset ? m_SourceAsset->samplerDesc.repeat : TextureWrap::Default; }

    TextureAnisotropy GetTextureAnisotropy() const { return IsOverridden(MatField::TextureAnisotropyMode) ? m_Overrides->textureAnisotropy : (m_SourceAsset ? m_SourceAsset->samplerDesc.aniso : TextureAnisotropy::Default); }
    void SetTextureAnisotropy(TextureAnisotropy a) { EnsureOverrides().textureAnisotropy = a; SetOverride(MatField::TextureAnisotropyMode); m_Sampler.GetDesc().aniso = a; }
    void ClearTextureAnisotropy() { ClearOverride(MatField::TextureAnisotropyMode); m_Sampler.GetDesc().aniso = m_SourceAsset ? m_SourceAsset->samplerDesc.aniso : TextureAnisotropy::Default; }

    bool IsOverridden(MatField f) const { return m_Overrides ? m_Overrides->overrideMask.test(static_cast<size_t>(f)) : false; }
    bool HasAnyOverride() const { return m_Overrides ? (m_Overrides->overrideMask.any() || (m_Overrides->customUniformOverrides && !m_Overrides->customUniformOverrides->empty())) : false; }
    void ClearAllOverrides() { m_Overrides.reset(); }
    bool IsValid() const { return m_SourceAsset.Get() != nullptr; }
    void RebuildCache();

    nlohmann::json SerializeOverrides() const;
    void DeserializeOverrides(const nlohmann::json& j);

private:
    MaterialOverrideData& EnsureOverrides();
    void SetOverride(MatField f);
    void ClearOverride(MatField f);

    friend struct MaterialComponent;

    Ref<MaterialAsset> m_SourceAsset;
    std::unique_ptr<MaterialOverrideData> m_Overrides;
    Sampler m_Sampler;
};

}
