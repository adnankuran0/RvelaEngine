#pragma once
#include "Asset/AssetUUID.h"
#include "Rendering/MaterialInstance.h"
#include "Asset/AssetManager.h"
#include "json.hpp"

namespace rv {

struct MaterialComponent
{
public:
    MaterialComponent() = default;
    explicit MaterialComponent(const AssetUUID& uuid) { Load(uuid); }

    MaterialInstance& GetInstance() { return m_Instance; }
    const MaterialInstance& GetInstance() const { return m_Instance; }

    const AssetUUID& GetMaterialUUID() const { return m_MaterialUUID; }

    void SetMaterial(const AssetUUID& uuid) { Load(uuid); }
    void Reload() { Load(m_MaterialUUID); }

    glm::vec3 GetAlbedoColor() const { return m_Instance.AlbedoColor(); }
    void SetAlbedoColor(const glm::vec3& v) { m_Instance.AlbedoColor() = v; }

    glm::vec3 GetEmissiveColor() const { return m_Instance.EmissiveColor(); }
    void SetEmissiveColor(const glm::vec3& v) { m_Instance.EmissiveColor() = v; }

    float GetEmissiveIntensity() const { return m_Instance.EmissiveIntensity(); }
    void  SetEmissiveIntensity(float v) { m_Instance.EmissiveIntensity() = v; }

    float GetMetallic() const { return m_Instance.Metallic(); }
    void  SetMetallic(float v) { m_Instance.Metallic() = v; }

    float GetSpecular() const { return m_Instance.Specular(); }
    void  SetSpecular(float v) { m_Instance.Specular() = v; }

    float GetRoughness() const { return m_Instance.Roughness(); }
    void  SetRoughness(float v) { m_Instance.Roughness() = v; }

    float GetAO() const { return m_Instance.AO(); }
    void  SetAO(float v) { m_Instance.AO() = v; }

    float GetNormalScale() const { return m_Instance.NormalScale(); }
    void  SetNormalScale(float v) { m_Instance.NormalScale() = v; }

    float GetHeightScale() const { return m_Instance.HeightScale(); }
    void  SetHeightScale(float v) { m_Instance.HeightScale() = v; }

    glm::vec2 GetUVScale() const { return m_Instance.UVScale(); }
    void SetUVScale(const glm::vec2& v) { m_Instance.UVScale() = v; }

    glm::vec2 GetUVOffset() const { return m_Instance.UVOffset(); }
    void SetUVOffset(const glm::vec2& v) { m_Instance.UVOffset() = v; }

    bool IsUsingAlbedoMap() const { return m_Instance.UsesAlbedoMap(); }
    bool IsUsingNormalMap() const { return m_Instance.UsesNormalMap(); }
    bool IsUsingMetallicMap() const { return m_Instance.UsesMetallicMap(); }
    bool IsUsingRoughnessMap() const { return m_Instance.UsesRoughnessMap(); }
    bool IsUsingAOMap() const { return m_Instance.UsesAOMap(); }
    bool IsUsingHeightMap() const { return m_Instance.UsesHeightMap(); }

    Ref<TextureAsset> GetAlbedoTexture() const { return m_Instance.GetAlbedoTexture(); }
    Ref<TextureAsset> GetNormalTexture() const { return m_Instance.GetNormalTexture(); }
    Ref<TextureAsset> GetMetallicTexture() const { return m_Instance.GetMetallicTexture(); }
    Ref<TextureAsset> GetRoughnessTexture() const { return m_Instance.GetRoughnessTex(); }
    Ref<TextureAsset> GetAOTexture() const { return m_Instance.GetAOTexture(); }
    Ref<TextureAsset> GetHeightTexture() const { return m_Instance.GetHeightTexture(); }

    void SetAlbedoTexture(const AssetUUID& uuid)
    {
        m_Instance.SetAlbedoTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid));
    }
    void SetNormalTexture(const AssetUUID& uuid)
    {
        m_Instance.SetNormalTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid));
    }
    void SetMetallicTexture(const AssetUUID& uuid)
    {
        m_Instance.SetMetallicTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid));
    }
    void SetRoughnessTexture(const AssetUUID& uuid)
    {
        m_Instance.SetRoughnessTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid));
    }
    void SetAOTexture(const AssetUUID& uuid)
    {
        m_Instance.SetAOTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid));
    }
    void SetHeightTexture(const AssetUUID& uuid)
    {
        m_Instance.SetHeightTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid));
    }
    void SetUseAlbedoMap(bool v) { m_Instance.m_UseAlbedoMap = v; }
    void SetUseNormalMap(bool v) { m_Instance.m_UseNormalMap = v; }
    void SetUseMetallicMap(bool v) { m_Instance.m_UseMetallicMap = v; }
    void SetUseRoughnessMap(bool v) { m_Instance.m_UseRoughnessMap = v; }
    void SetUseAOMap(bool v) { m_Instance.m_UseAOMap = v; }
    void SetUseHeightMap(bool v) { m_Instance.m_UseHeightMap = v; }

    Sampler& GetSampler() { return m_Instance.GetSampler(); }
    const Sampler& GetSampler() const { return m_Instance.GetSampler(); }

    nlohmann::json Serialize() const;
    void Deserialize(const nlohmann::json& j);


private:
    void Load(const AssetUUID& uuid);

    AssetUUID m_MaterialUUID;
    MaterialInstance m_Instance;
};

}