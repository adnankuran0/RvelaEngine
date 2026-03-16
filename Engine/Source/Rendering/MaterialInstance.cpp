#include "rvelapch.h"
#include "MaterialInstance.h"
#include "Asset/AssetManager.h"

using namespace rv;

MaterialInstance MaterialInstance::CreateFromAsset(const Ref<MaterialAsset>& asset)
{
    MaterialInstance inst;
    if (!asset) return inst;

    inst.m_SourceUUID = asset->GetUUID();
    inst.m_AlbedoColor = asset->albedoColor;
    inst.m_EmissiveColor = asset->emissiveColor;
    inst.m_EmissiveIntensity = asset->emissiveIntensity;
    inst.m_Metallic = asset->metallic;
    inst.m_Specular = asset->specular;
    inst.m_Roughness = asset->roughness;
    inst.m_AO = asset->ao;
    inst.m_NormalScale = asset->normalScale;
    inst.m_HeightScale = asset->heightScale;
    inst.m_UVScale = asset->UVScale;
    inst.m_UVOffset = asset->UVOffset;
    inst.m_UseAlbedoMap = asset->useAlbedoMap;
    inst.m_UseNormalMap = asset->useNormalMap;
    inst.m_UseMetallicMap = asset->useMetallicMap;
    inst.m_UseRoughnessMap = asset->useRoughnessMap;
    inst.m_UseAOMap = asset->useAOMap;
    inst.m_UseHeightMap = asset->useHeightMap;
    inst.m_Sampler.Init(asset->samplerDesc);

    auto& mgr = AssetManager::Get();

    if (asset->useAlbedoMap)
        inst.m_AlbedoTex = mgr.GetAsset<TextureAsset>(asset->albedoTextureUUID);
    if (asset->useNormalMap)
        inst.m_NormalTex = mgr.GetAsset<TextureAsset>(asset->normalTextureUUID);
    if (asset->useMetallicMap)
        inst.m_MetallicTex = mgr.GetAsset<TextureAsset>(asset->metallicTextureUUID);
    if (asset->useRoughnessMap)
        inst.m_RoughnessTex = mgr.GetAsset<TextureAsset>(asset->roughnessTextureUUID);
    if (asset->useAOMap)
        inst.m_AOTex = mgr.GetAsset<TextureAsset>(asset->aoTextureUUID);
    if (asset->useHeightMap)
        inst.m_HeightTex = mgr.GetAsset<TextureAsset>(asset->heightTextureUUID);

    return inst;
}