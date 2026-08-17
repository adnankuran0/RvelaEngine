#include "rvelapch.h"
#include "MaterialInstance.h"
#include "Asset/AssetManager.h"

using namespace rv;
using json = nlohmann::json;

MaterialInstance MaterialInstance::CreateFromAsset(const Ref<MaterialAsset>& asset)
{
    MaterialInstance inst;
    if (!asset) return inst;
    inst.m_SourceAsset = asset;
    inst.m_Sampler.Init(asset->samplerDesc);
    inst.RebuildCache();
    return inst;
}

json MaterialInstance::SerializeOverrides() const
{
    json j;

    if (IsOverridden(MatField::AlbedoColor))
        j["albedoColor"] = { m_AlbedoColor.r, m_AlbedoColor.g, m_AlbedoColor.b, m_AlbedoColor.a };
    if (IsOverridden(MatField::EmissiveColor))
        j["emissiveColor"] = { m_EmissiveColor.r, m_EmissiveColor.g, m_EmissiveColor.b };
    if (IsOverridden(MatField::EmissiveIntensity))
        j["emissiveIntensity"] = m_EmissiveIntensity;
    if (IsOverridden(MatField::Metallic))
        j["metallic"] = m_Metallic;
    if (IsOverridden(MatField::Specular))
        j["specular"] = m_Specular;
    if (IsOverridden(MatField::Roughness))
        j["roughness"] = m_Roughness;
    if (IsOverridden(MatField::AO))
        j["ao"] = m_AO;
    if (IsOverridden(MatField::NormalScale))
        j["normalScale"] = m_NormalScale;
    if (IsOverridden(MatField::HeightScale))
        j["heightScale"] = m_HeightScale;
    if (IsOverridden(MatField::UVScale))
        j["UVScale"] = { m_UVScale.x, m_UVScale.y };
    if (IsOverridden(MatField::UVOffset))
        j["UVOffset"] = { m_UVOffset.x, m_UVOffset.y };

    if (IsOverridden(MatField::TransparencyMode))
        j["transparencyMode"] = static_cast<int>(m_TransparencyMode);
    if (IsOverridden(MatField::BlendMode))
        j["blendMode"] = static_cast<int>(m_BlendMode);
    if (IsOverridden(MatField::CullMode))
        j["cullMode"] = static_cast<int>(m_CullMode);
    if (IsOverridden(MatField::AlphaCutoff))
        j["alphaCutoff"] = m_AlphaCutoff;

    if (IsOverridden(MatField::AlbedoTex))
        j["albedoTex"] = m_AlbedoTex ? m_AlbedoTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::NormalTex))
        j["normalTex"] = m_NormalTex ? m_NormalTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::MetallicTex))
        j["metallicTex"] = m_MetallicTex ? m_MetallicTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::RoughnessTex))
        j["roughnessTex"] = m_RoughnessTex ? m_RoughnessTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::AOTex))
        j["aoTex"] = m_AOTex ? m_AOTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::HeightTex))
        j["heightTex"] = m_HeightTex ? m_HeightTex->GetUUID().ToString() : "";

    return j;
}

void MaterialInstance::DeserializeOverrides(const json& j)
{
    auto& mgr = AssetManager::Get();

    if (j.contains("albedoColor")) {
        auto& col = j["albedoColor"];
        m_AlbedoColor = { col[0], col[1], col[2], col.size() > 3 ? col[3].get<float>() : 1.0f };
        SetOverride(MatField::AlbedoColor);
    }
    if (j.contains("emissiveColor"))
    {
        m_EmissiveColor = { j["emissiveColor"][0], j["emissiveColor"][1], j["emissiveColor"][2] };
        SetOverride(MatField::EmissiveColor);
    }
    if (j.contains("emissiveIntensity"))
    {
        m_EmissiveIntensity = j["emissiveIntensity"];
        SetOverride(MatField::EmissiveIntensity);
    }
    if (j.contains("metallic"))
    {
        m_Metallic = j["metallic"];
        SetOverride(MatField::Metallic);
    }
    if (j.contains("specular"))
    {
        m_Specular = j["specular"];
        SetOverride(MatField::Specular);
    }
    if (j.contains("roughness"))
    {
        m_Roughness = j["roughness"];
        SetOverride(MatField::Roughness);
    }
    if (j.contains("ao"))
    {
        m_AO = j["ao"];
        SetOverride(MatField::AO);
    }
    if (j.contains("normalScale"))
    {
        m_NormalScale = j["normalScale"];
        SetOverride(MatField::NormalScale);
    }
    if (j.contains("heightScale"))
    {
        m_HeightScale = j["heightScale"];
        SetOverride(MatField::HeightScale);
    }
    if (j.contains("UVScale"))
    {
        m_UVScale = { j["UVScale"][0], j["UVScale"][1] };
        SetOverride(MatField::UVScale);
    }
    if (j.contains("UVOffset"))
    {
        m_UVOffset = { j["UVOffset"][0], j["UVOffset"][1] };
        SetOverride(MatField::UVOffset);
    }

    if (j.contains("transparencyMode"))
    {
        m_TransparencyMode = static_cast<TransparencyMode>(j["transparencyMode"].get<int>());
        SetOverride(MatField::TransparencyMode);
    }
    if (j.contains("blendMode"))
    {
        m_BlendMode = static_cast<BlendMode>(j["blendMode"].get<int>());
        SetOverride(MatField::BlendMode);
    }
    if (j.contains("cullMode"))
    {
        m_CullMode = static_cast<CullMode>(j["cullMode"].get<int>());
        SetOverride(MatField::CullMode);
    }
    if (j.contains("alphaCutoff"))
    {
        m_AlphaCutoff = j["alphaCutoff"].get<float>();
        SetOverride(MatField::AlphaCutoff);
    }

    auto loadTex = [&](const char* key, Ref<TextureAsset>& target, bool& useFlag, MatField field)
        {
            if (!j.contains(key)) return;
            std::string uuidStr = j[key].get<std::string>();
            if (!uuidStr.empty())
            {
                auto uuid = AssetUUID::FromString(uuidStr);
                target = mgr.GetAsset<TextureAsset>(uuid);
                useFlag = static_cast<bool>(target);
            }
            else
            {
                target = nullptr;
                useFlag = false;
            }
            SetOverride(field);
        };

    loadTex("albedoTex", m_AlbedoTex, m_UseAlbedoMap, MatField::AlbedoTex);
    loadTex("normalTex", m_NormalTex, m_UseNormalMap, MatField::NormalTex);
    loadTex("metallicTex", m_MetallicTex, m_UseMetallicMap, MatField::MetallicTex);
    loadTex("roughnessTex", m_RoughnessTex, m_UseRoughnessMap, MatField::RoughnessTex);
    loadTex("aoTex", m_AOTex, m_UseAOMap, MatField::AOTex);
    loadTex("heightTex", m_HeightTex, m_UseHeightMap, MatField::HeightTex);
}

void MaterialInstance::RebuildCache()
{
    if (!m_SourceAsset) return;
    auto& mgr = AssetManager::Get();
    m_CachedAlbedoTex = m_SourceAsset->useAlbedoMap ? mgr.GetAsset<TextureAsset>(m_SourceAsset->albedoTextureUUID) : nullptr;
    m_CachedNormalTex = m_SourceAsset->useNormalMap ? mgr.GetAsset<TextureAsset>(m_SourceAsset->normalTextureUUID) : nullptr;
    m_CachedMetallicTex = m_SourceAsset->useMetallicMap ? mgr.GetAsset<TextureAsset>(m_SourceAsset->metallicTextureUUID) : nullptr;
    m_CachedRoughnessTex = m_SourceAsset->useRoughnessMap ? mgr.GetAsset<TextureAsset>(m_SourceAsset->roughnessTextureUUID) : nullptr;
    m_CachedAOTex = m_SourceAsset->useAOMap ? mgr.GetAsset<TextureAsset>(m_SourceAsset->aoTextureUUID) : nullptr;
    m_CachedHeightTex = m_SourceAsset->useHeightMap ? mgr.GetAsset<TextureAsset>(m_SourceAsset->heightTextureUUID) : nullptr;
}