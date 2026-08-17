#pragma once
#include "Asset/AssetUUID.h"
#include "Asset/Types/MaterialAsset.h"
#include "Asset/Types/TextureAsset.h"
#include "Core/Ref.h"
#include "Renderer/Sampler.h"
#include <glm/glm.hpp>
#include <bitset>
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
    UVScale,
    UVOffset,
    AlbedoTex,
    NormalTex,
    MetallicTex,
    RoughnessTex,
    AOTex,
    HeightTex,
    TransparencyMode,
    BlendMode,
    CullMode,
    AlphaCutoff,
    ShadingMode,
    ReceiveShadows,
    BillboardMode,
    COUNT
};

class MaterialInstance
{
public:
    static MaterialInstance CreateFromAsset(const Ref<MaterialAsset>& asset);

    Ref<MaterialAsset> GetSourceAsset() const { return m_SourceAsset; }
    AssetUUID GetSourceUUID() const { return m_SourceAsset ? m_SourceAsset->GetUUID() : AssetUUID{}; }

    Ref<TextureAsset> GetAlbedoTexture() const { return IsOverridden(MatField::AlbedoTex) ? m_AlbedoTex : m_CachedAlbedoTex; }
    Ref<TextureAsset> GetNormalTexture() const { return IsOverridden(MatField::NormalTex) ? m_NormalTex : m_CachedNormalTex; }
    Ref<TextureAsset> GetMetallicTexture() const { return IsOverridden(MatField::MetallicTex) ? m_MetallicTex : m_CachedMetallicTex; }
    Ref<TextureAsset> GetRoughnessTex() const { return IsOverridden(MatField::RoughnessTex) ? m_RoughnessTex : m_CachedRoughnessTex; }
    Ref<TextureAsset> GetAOTexture() const { return IsOverridden(MatField::AOTex) ? m_AOTex : m_CachedAOTex; }
    Ref<TextureAsset> GetHeightTexture() const { return IsOverridden(MatField::HeightTex) ? m_HeightTex : m_CachedHeightTex; }

    void SetAlbedoTexture(Ref<TextureAsset> tex) { m_AlbedoTex = tex; m_UseAlbedoMap = static_cast<bool>(tex); SetOverride(MatField::AlbedoTex); }
    void SetNormalTexture(Ref<TextureAsset> tex) { m_NormalTex = tex; m_UseNormalMap = static_cast<bool>(tex); SetOverride(MatField::NormalTex); }
    void SetMetallicTexture(Ref<TextureAsset> tex) { m_MetallicTex = tex; m_UseMetallicMap = static_cast<bool>(tex); SetOverride(MatField::MetallicTex); }
    void SetRoughnessTexture(Ref<TextureAsset> tex) { m_RoughnessTex = tex; m_UseRoughnessMap = static_cast<bool>(tex); SetOverride(MatField::RoughnessTex); }
    void SetAOTexture(Ref<TextureAsset> tex) { m_AOTex = tex; m_UseAOMap = static_cast<bool>(tex); SetOverride(MatField::AOTex); }
    void SetHeightTexture(Ref<TextureAsset> tex) { m_HeightTex = tex; m_UseHeightMap = static_cast<bool>(tex); SetOverride(MatField::HeightTex); }

    void SetAlbedoColor(const glm::vec4& v) { m_AlbedoColor = v; SetOverride(MatField::AlbedoColor); }
    void SetEmissiveColor(const glm::vec3& v) { m_EmissiveColor = v; SetOverride(MatField::EmissiveColor); }
    void SetEmissiveIntensity(float v) { m_EmissiveIntensity = v; SetOverride(MatField::EmissiveIntensity); }
    void SetMetallic(float v) { m_Metallic = v; SetOverride(MatField::Metallic); }
    void SetSpecular(float v) { m_Specular = v; SetOverride(MatField::Specular); }
    void SetRoughness(float v) { m_Roughness = v; SetOverride(MatField::Roughness); }
    void SetAO(float v) { m_AO = v; SetOverride(MatField::AO); }
    void SetNormalScale(float v) { m_NormalScale = v; SetOverride(MatField::NormalScale); }
    void SetHeightScale(float v) { m_HeightScale = v; SetOverride(MatField::HeightScale); }
    void SetUVScale(const glm::vec2& v) { m_UVScale = v; SetOverride(MatField::UVScale); }
    void SetUVOffset(const glm::vec2& v) { m_UVOffset = v; SetOverride(MatField::UVOffset); }

    void SetTransparencyMode(TransparencyMode mode) { m_TransparencyMode = mode; SetOverride(MatField::TransparencyMode); }
    void SetBlendMode(BlendMode mode) { m_BlendMode = mode; SetOverride(MatField::BlendMode); }
    void SetCullMode(CullMode mode) { m_CullMode = mode; SetOverride(MatField::CullMode); }
    void SetAlphaCutoff(float v) { m_AlphaCutoff = v; SetOverride(MatField::AlphaCutoff); }

    void SetBillboardMode(BillboardMode mode) { m_BillboardMode = mode; SetOverride(MatField::BillboardMode); }

    void SetShadingMode(ShadingMode mode) { m_ShadingMode = mode; SetOverride(MatField::ShadingMode); }
    void SetReceiveShadows(bool v) { m_ReceiveShadows = v; SetOverride(MatField::ReceiveShadows); }

    glm::vec4 GetAlbedoColor() const { return IsOverridden(MatField::AlbedoColor) ? m_AlbedoColor : (m_SourceAsset ? m_SourceAsset->albedoColor : m_AlbedoColor); }
    glm::vec3 GetEmissiveColor() const { return IsOverridden(MatField::EmissiveColor) ? m_EmissiveColor : (m_SourceAsset ? m_SourceAsset->emissiveColor : m_EmissiveColor); }
    float GetEmissiveIntensity() const { return IsOverridden(MatField::EmissiveIntensity) ? m_EmissiveIntensity : (m_SourceAsset ? m_SourceAsset->emissiveIntensity : m_EmissiveIntensity); }
    float GetMetallic() const { return IsOverridden(MatField::Metallic) ? m_Metallic : (m_SourceAsset ? m_SourceAsset->metallic : m_Metallic); }
    float GetSpecular() const { return IsOverridden(MatField::Specular) ? m_Specular : (m_SourceAsset ? m_SourceAsset->specular : m_Specular); }
    float GetRoughness() const { return IsOverridden(MatField::Roughness) ? m_Roughness : (m_SourceAsset ? m_SourceAsset->roughness : m_Roughness); }
    float GetAO() const { return IsOverridden(MatField::AO) ? m_AO : (m_SourceAsset ? m_SourceAsset->ao : m_AO); }
    float GetNormalScale() const { return IsOverridden(MatField::NormalScale) ? m_NormalScale : (m_SourceAsset ? m_SourceAsset->normalScale : m_NormalScale); }
    float GetHeightScale() const { return IsOverridden(MatField::HeightScale) ? m_HeightScale : (m_SourceAsset ? m_SourceAsset->heightScale : m_HeightScale); }
    glm::vec2 GetUVScale() const { return IsOverridden(MatField::UVScale) ? m_UVScale : (m_SourceAsset ? m_SourceAsset->UVScale : m_UVScale); }
    glm::vec2 GetUVOffset() const { return IsOverridden(MatField::UVOffset) ? m_UVOffset : (m_SourceAsset ? m_SourceAsset->UVOffset : m_UVOffset); }

    TransparencyMode GetTransparencyMode() const { return IsOverridden(MatField::TransparencyMode) ? m_TransparencyMode : (m_SourceAsset ? m_SourceAsset->transparencyMode : m_TransparencyMode); }
    BlendMode GetBlendMode() const { return IsOverridden(MatField::BlendMode) ? m_BlendMode : (m_SourceAsset ? m_SourceAsset->blendMode : m_BlendMode); }
    CullMode GetCullMode() const { return IsOverridden(MatField::CullMode) ? m_CullMode : (m_SourceAsset ? m_SourceAsset->cullMode : m_CullMode); }
    float GetAlphaCutoff() const { return IsOverridden(MatField::AlphaCutoff) ? m_AlphaCutoff : (m_SourceAsset ? m_SourceAsset->alphaCutoff : m_AlphaCutoff); }

    BillboardMode GetBillboardMode() const { return IsOverridden(MatField::BillboardMode) ? m_BillboardMode : (m_SourceAsset ? m_SourceAsset->billboardMode : m_BillboardMode); }

    ShadingMode GetShadingMode() const { return IsOverridden(MatField::ShadingMode) ? m_ShadingMode : (m_SourceAsset ? m_SourceAsset->shadingMode : m_ShadingMode); }
    bool GetReceiveShadows() const { return IsOverridden(MatField::ReceiveShadows) ? m_ReceiveShadows : (m_SourceAsset ? m_SourceAsset->receiveShadows : m_ReceiveShadows); }

    bool UsesAlbedoMap() const { return IsOverridden(MatField::AlbedoTex) ? m_UseAlbedoMap : (m_SourceAsset ? m_SourceAsset->useAlbedoMap : false); }
    bool UsesNormalMap() const { return IsOverridden(MatField::NormalTex) ? m_UseNormalMap : (m_SourceAsset ? m_SourceAsset->useNormalMap : false); }
    bool UsesMetallicMap() const { return IsOverridden(MatField::MetallicTex) ? m_UseMetallicMap : (m_SourceAsset ? m_SourceAsset->useMetallicMap : false); }
    bool UsesRoughnessMap() const { return IsOverridden(MatField::RoughnessTex) ? m_UseRoughnessMap : (m_SourceAsset ? m_SourceAsset->useRoughnessMap : false); }
    bool UsesAOMap() const { return IsOverridden(MatField::AOTex) ? m_UseAOMap : (m_SourceAsset ? m_SourceAsset->useAOMap : false); }
    bool UsesHeightMap() const { return IsOverridden(MatField::HeightTex) ? m_UseHeightMap : (m_SourceAsset ? m_SourceAsset->useHeightMap : false); }

    void ClearAlbedoColor() { ClearOverride(MatField::AlbedoColor); }
    void ClearEmissiveColor() { ClearOverride(MatField::EmissiveColor); }
    void ClearEmissiveIntensity() { ClearOverride(MatField::EmissiveIntensity); }
    void ClearMetallic() { ClearOverride(MatField::Metallic); }
    void ClearSpecular() { ClearOverride(MatField::Specular); }
    void ClearRoughness() { ClearOverride(MatField::Roughness); }
    void ClearAO() { ClearOverride(MatField::AO); }
    void ClearNormalScale() { ClearOverride(MatField::NormalScale); }
    void ClearHeightScale() { ClearOverride(MatField::HeightScale); }
    void ClearUVScale() { ClearOverride(MatField::UVScale); }
    void ClearUVOffset() { ClearOverride(MatField::UVOffset); }
    void ClearAlbedoTexture() { m_AlbedoTex = nullptr; m_UseAlbedoMap = false; ClearOverride(MatField::AlbedoTex); }
    void ClearNormalTexture() { m_NormalTex = nullptr; m_UseNormalMap = false; ClearOverride(MatField::NormalTex); }
    void ClearMetallicTexture() { m_MetallicTex = nullptr; m_UseMetallicMap = false; ClearOverride(MatField::MetallicTex); }
    void ClearRoughnessTexture() { m_RoughnessTex = nullptr; m_UseRoughnessMap = false; ClearOverride(MatField::RoughnessTex); }
    void ClearAOTexture() { m_AOTex = nullptr; m_UseAOMap = false; ClearOverride(MatField::AOTex); }
    void ClearHeightTexture() { m_HeightTex = nullptr; m_UseHeightMap = false; ClearOverride(MatField::HeightTex); }

    void ClearTransparencyMode() { ClearOverride(MatField::TransparencyMode); }
    void ClearBlendMode() { ClearOverride(MatField::BlendMode); }
    void ClearCullMode() { ClearOverride(MatField::CullMode); }
    void ClearAlphaCutoff() { ClearOverride(MatField::AlphaCutoff); }

    void ClearBillboardMode() { ClearOverride(MatField::BillboardMode); }

    void ClearShadingMode() { ClearOverride(MatField::ShadingMode); }
    void ClearReceiveShadows() { ClearOverride(MatField::ReceiveShadows); }

    Sampler& GetSampler() { return m_Sampler; }
    const Sampler& GetSampler() const { return m_Sampler; }

    bool IsOverridden(MatField f) const { return m_OverrideMask.test((size_t)f); }
    bool HasAnyOverride() const { return m_OverrideMask.any(); }
    void ClearAllOverrides() { m_OverrideMask.reset(); }
    bool IsValid() const { return m_SourceAsset.Get() != nullptr; }
    void RebuildCache();

    nlohmann::json SerializeOverrides() const;
    void DeserializeOverrides(const nlohmann::json& j);

private:
    void SetOverride(MatField f) { m_OverrideMask.set((size_t)f); }
    void ClearOverride(MatField f) { m_OverrideMask.reset((size_t)f); }

    friend struct MaterialComponent;

    Ref<MaterialAsset> m_SourceAsset;
    std::bitset<(size_t)MatField::COUNT> m_OverrideMask;

    Ref<TextureAsset> m_AlbedoTex;
    Ref<TextureAsset> m_NormalTex;
    Ref<TextureAsset> m_MetallicTex;
    Ref<TextureAsset> m_RoughnessTex;
    Ref<TextureAsset> m_AOTex;
    Ref<TextureAsset> m_HeightTex;

    Ref<TextureAsset> m_CachedAlbedoTex;
    Ref<TextureAsset> m_CachedNormalTex;
    Ref<TextureAsset> m_CachedMetallicTex;
    Ref<TextureAsset> m_CachedRoughnessTex;
    Ref<TextureAsset> m_CachedAOTex;
    Ref<TextureAsset> m_CachedHeightTex;

    glm::vec4 m_AlbedoColor = glm::vec4(1.0f);
    glm::vec3 m_EmissiveColor = glm::vec3(0.0f);
    float m_EmissiveIntensity = 0.0f;
    float m_Metallic = 0.0f;
    float m_Specular = 1.0f;
    float m_Roughness = 1.0f;
    float m_AO = 1.0f;
    float m_NormalScale = 1.0f;
    float m_HeightScale = 0.1f;
    glm::vec2 m_UVScale = glm::vec2(1.0f);
    glm::vec2 m_UVOffset = glm::vec2(0.0f);

    TransparencyMode m_TransparencyMode = TransparencyMode::Opaque;
    BlendMode m_BlendMode = BlendMode::Mix;
    CullMode m_CullMode = CullMode::Back;
    float m_AlphaCutoff = 0.5f;

    BillboardMode m_BillboardMode = BillboardMode::Disabled;

    ShadingMode m_ShadingMode = ShadingMode::Lit;
    bool m_ReceiveShadows = true;

    bool m_UseAlbedoMap = false;
    bool m_UseNormalMap = false;
    bool m_UseMetallicMap = false;
    bool m_UseRoughnessMap = false;
    bool m_UseAOMap = false;
    bool m_UseHeightMap = false;

    Sampler m_Sampler;
};

}