#pragma once
#include "Asset/AssetUUID.h"
#include "Asset/Types/MaterialAsset.h"
#include "Asset/Types/TextureAsset.h"
#include "Core/Ref.h"
#include "Rendering/Sampler.h"
#include <glm/glm.hpp>

namespace rv {

class MaterialInstance
{
public:
    static MaterialInstance CreateFromAsset(const Ref<MaterialAsset>& asset);

    AssetUUID GetSourceUUID() const { return m_SourceUUID; }

    Ref<TextureAsset> GetAlbedoTexture() const { return m_AlbedoTex; }
    Ref<TextureAsset> GetNormalTexture() const { return m_NormalTex; }
    Ref<TextureAsset> GetMetallicTexture() const { return m_MetallicTex; }
    Ref<TextureAsset> GetRoughnessTex() const { return m_RoughnessTex; }
    Ref<TextureAsset> GetAOTexture() const { return m_AOTex; }
    Ref<TextureAsset> GetHeightTexture() const { return m_HeightTex; }

    void SetAlbedoTexture(Ref<TextureAsset> tex) { m_AlbedoTex = tex; m_UseAlbedoMap = static_cast<bool>(tex); }
    void SetNormalTexture(Ref<TextureAsset> tex) { m_NormalTex = tex; m_UseNormalMap = static_cast<bool>(tex); }
    void SetMetallicTexture(Ref<TextureAsset> tex) { m_MetallicTex = tex; m_UseMetallicMap = static_cast<bool>(tex); }
    void SetRoughnessTexture(Ref<TextureAsset> tex) { m_RoughnessTex = tex; m_UseRoughnessMap = static_cast<bool>(tex); }
    void SetAOTexture(Ref<TextureAsset> tex) { m_AOTex = tex; m_UseAOMap = static_cast<bool>(tex); }
    void SetHeightTexture(Ref<TextureAsset> tex) { m_HeightTex = tex; m_UseHeightMap = static_cast<bool>(tex); }

    glm::vec3& AlbedoColor() { return m_AlbedoColor; }
    glm::vec3& EmissiveColor() { return m_EmissiveColor; }
    float& EmissiveIntensity() { return m_EmissiveIntensity; }
    float& Metallic() { return m_Metallic; }
    float& Specular() { return m_Specular; }
    float& Roughness() { return m_Roughness; }
    float& AO() { return m_AO; }
    float& NormalScale() { return m_NormalScale; }
    float& HeightScale() { return m_HeightScale; }
    glm::vec2& UVScale() { return m_UVScale; }
    glm::vec2& UVOffset() { return m_UVOffset; }

    const glm::vec3& AlbedoColor() const { return m_AlbedoColor; }
    const glm::vec3& EmissiveColor() const { return m_EmissiveColor; }
    float EmissiveIntensity() const { return m_EmissiveIntensity; }
    float Metallic() const { return m_Metallic; }
    float Specular() const { return m_Specular; }
    float Roughness() const { return m_Roughness; }
    float AO() const { return m_AO; }
    float NormalScale() const { return m_NormalScale; }
    float HeightScale() const { return m_HeightScale; }
    const glm::vec2& UVScale() const { return m_UVScale; }
    const glm::vec2& UVOffset() const { return m_UVOffset; }

    bool UsesAlbedoMap() const { return m_UseAlbedoMap; }
    bool UsesNormalMap() const { return m_UseNormalMap; }
    bool UsesMetallicMap() const { return m_UseMetallicMap; }
    bool UsesRoughnessMap() const { return m_UseRoughnessMap; }
    bool UsesAOMap() const { return m_UseAOMap; }
    bool UsesHeightMap() const { return m_UseHeightMap; }

    Sampler& GetSampler() { return m_Sampler; }
    const Sampler& GetSampler() const { return m_Sampler; }

    bool IsValid() const { return m_SourceUUID.IsValid(); }

private:
    friend struct MaterialComponent;

    AssetUUID m_SourceUUID;

    Ref<TextureAsset> m_AlbedoTex;
    Ref<TextureAsset> m_NormalTex;
    Ref<TextureAsset> m_MetallicTex;
    Ref<TextureAsset> m_RoughnessTex;
    Ref<TextureAsset> m_AOTex;
    Ref<TextureAsset> m_HeightTex;

    glm::vec3 m_AlbedoColor = glm::vec3(1.0f);
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

    bool m_UseAlbedoMap = false;
    bool m_UseNormalMap = false;
    bool m_UseMetallicMap = false;
    bool m_UseRoughnessMap = false;
    bool m_UseAOMap = false;
    bool m_UseHeightMap = false;

    Sampler m_Sampler;
};

}