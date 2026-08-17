#pragma once
#include "Asset/AssetUUID.h"
#include "Renderer/MaterialInstance.h"
#include "Asset/AssetManager.h"
#include <nlohmann/json.hpp>

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

    glm::vec4 GetAlbedoColor() const { return m_Instance.GetAlbedoColor(); }
    void SetAlbedoColor(const glm::vec4& v) { m_Instance.SetAlbedoColor(v); }
    void ClearAlbedoColor() { m_Instance.ClearAlbedoColor(); }

    glm::vec3 GetEmissiveColor() const { return m_Instance.GetEmissiveColor(); }
    void SetEmissiveColor(const glm::vec3& v) { m_Instance.SetEmissiveColor(v); }
    void ClearEmissiveColor() { m_Instance.ClearEmissiveColor(); }

    float GetEmissiveIntensity() const { return m_Instance.GetEmissiveIntensity(); }
    void SetEmissiveIntensity(float v) { m_Instance.SetEmissiveIntensity(v); }
    void ClearEmissiveIntensity() { m_Instance.ClearEmissiveIntensity(); }

    float GetMetallic() const { return m_Instance.GetMetallic(); }
    void SetMetallic(float v) { m_Instance.SetMetallic(v); }
    void ClearMetallic() { m_Instance.ClearMetallic(); }

    float GetSpecular() const { return m_Instance.GetSpecular(); }
    void SetSpecular(float v) { m_Instance.SetSpecular(v); }
    void ClearSpecular() { m_Instance.ClearSpecular(); }

    float GetRoughness() const { return m_Instance.GetRoughness(); }
    void SetRoughness(float v) { m_Instance.SetRoughness(v); }
    void ClearRoughness() { m_Instance.ClearRoughness(); }

    float GetAO() const { return m_Instance.GetAO(); }
    void SetAO(float v) { m_Instance.SetAO(v); }
    void ClearAO() { m_Instance.ClearAO(); }

    float GetNormalScale() const { return m_Instance.GetNormalScale(); }
    void SetNormalScale(float v) { m_Instance.SetNormalScale(v); }
    void ClearNormalScale() { m_Instance.ClearNormalScale(); }

    float GetHeightScale() const { return m_Instance.GetHeightScale(); }
    void SetHeightScale(float v) { m_Instance.SetHeightScale(v); }
    void ClearHeightScale() { m_Instance.ClearHeightScale(); }

    glm::vec2 GetUVScale() const { return m_Instance.GetUVScale(); }
    void SetUVScale(const glm::vec2& v) { m_Instance.SetUVScale(v); }
    void ClearUVScale() { m_Instance.ClearUVScale(); }

    glm::vec2 GetUVOffset() const { return m_Instance.GetUVOffset(); }
    void SetUVOffset(const glm::vec2& v) { m_Instance.SetUVOffset(v); }
    void ClearUVOffset() { m_Instance.ClearUVOffset(); }

    TransparencyMode GetTransparencyMode() const { return m_Instance.GetTransparencyMode(); }
    void SetTransparencyMode(TransparencyMode mode) { m_Instance.SetTransparencyMode(mode); }
    void ClearTransparencyMode() { m_Instance.ClearTransparencyMode(); }

    BlendMode GetBlendMode() const { return m_Instance.GetBlendMode(); }
    void SetBlendMode(BlendMode mode) { m_Instance.SetBlendMode(mode); }
    void ClearBlendMode() { m_Instance.ClearBlendMode(); }

    CullMode GetCullMode() const { return m_Instance.GetCullMode(); }
    void SetCullMode(CullMode mode) { m_Instance.SetCullMode(mode); }
    void ClearCullMode() { m_Instance.ClearCullMode(); }

    float GetAlphaCutoff() const { return m_Instance.GetAlphaCutoff(); }
    void SetAlphaCutoff(float v) { m_Instance.SetAlphaCutoff(v); }
    void ClearAlphaCutoff() { m_Instance.ClearAlphaCutoff(); }

    ShadingMode GetShadingMode() const { return m_Instance.GetShadingMode(); }
    void SetShadingMode(ShadingMode mode) { m_Instance.SetShadingMode(mode); }
    void ClearShadingMode() { m_Instance.ClearShadingMode(); }

    bool GetReceiveShadows() const { return m_Instance.GetReceiveShadows(); }
    void SetReceiveShadows(bool v) { m_Instance.SetReceiveShadows(v); }
    void ClearReceiveShadows() { m_Instance.ClearReceiveShadows(); }

    BillboardMode GetBillboardMode() const { return m_Instance.GetBillboardMode(); }
    void SetBillboardMode(BillboardMode mode) { m_Instance.SetBillboardMode(mode); }
    void ClearBillboardMode() { m_Instance.ClearBillboardMode(); }

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

    void SetAlbedoTexture(const AssetUUID& uuid) { m_Instance.SetAlbedoTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid)); }
    void SetNormalTexture(const AssetUUID& uuid) { m_Instance.SetNormalTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid)); }
    void SetMetallicTexture(const AssetUUID& uuid) { m_Instance.SetMetallicTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid)); }
    void SetRoughnessTexture(const AssetUUID& uuid) { m_Instance.SetRoughnessTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid)); }
    void SetAOTexture(const AssetUUID& uuid) { m_Instance.SetAOTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid)); }
    void SetHeightTexture(const AssetUUID& uuid) { m_Instance.SetHeightTexture(AssetManager::Get().GetAsset<TextureAsset>(uuid)); }

    void ClearAlbedoTexture() { m_Instance.ClearAlbedoTexture(); }
    void ClearNormalTexture() { m_Instance.ClearNormalTexture(); }
    void ClearMetallicTexture() { m_Instance.ClearMetallicTexture(); }
    void ClearRoughnessTexture() { m_Instance.ClearRoughnessTexture(); }
    void ClearAOTexture() { m_Instance.ClearAOTexture(); }
    void ClearHeightTexture() { m_Instance.ClearHeightTexture(); }

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