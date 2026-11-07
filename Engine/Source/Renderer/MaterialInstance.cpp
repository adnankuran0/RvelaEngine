#include "rvelapch.h"
#include "MaterialInstance.h"
#include "Asset/AssetManager.h"
#include "Asset/Types/TextureAsset.h"
#include "Asset/Types/ShaderAsset.h"

using namespace rv;
using json = nlohmann::json;

Ref<TextureAsset> MaterialAsset::GetAlbedoTexture() const
{
    if (!useAlbedoMap || !albedoTextureUUID.IsValid()) return nullptr;
    if (!m_ResolvedAlbedoTex || m_ResolvedAlbedoTex->GetUUID() != albedoTextureUUID)
        m_ResolvedAlbedoTex = AssetManager::Get().GetAsset<TextureAsset>(albedoTextureUUID);
    return m_ResolvedAlbedoTex;
}

Ref<TextureAsset> MaterialAsset::GetNormalTexture() const
{
    if (!useNormalMap || !normalTextureUUID.IsValid()) return nullptr;
    if (!m_ResolvedNormalTex || m_ResolvedNormalTex->GetUUID() != normalTextureUUID)
        m_ResolvedNormalTex = AssetManager::Get().GetAsset<TextureAsset>(normalTextureUUID);
    return m_ResolvedNormalTex;
}

Ref<TextureAsset> MaterialAsset::GetMetallicTexture() const
{
    if (!useMetallicMap || !metallicTextureUUID.IsValid()) return nullptr;
    if (!m_ResolvedMetallicTex || m_ResolvedMetallicTex->GetUUID() != metallicTextureUUID)
        m_ResolvedMetallicTex = AssetManager::Get().GetAsset<TextureAsset>(metallicTextureUUID);
    return m_ResolvedMetallicTex;
}

Ref<TextureAsset> MaterialAsset::GetRoughnessTexture() const
{
    if (!useRoughnessMap || !roughnessTextureUUID.IsValid()) return nullptr;
    if (!m_ResolvedRoughnessTex || m_ResolvedRoughnessTex->GetUUID() != roughnessTextureUUID)
        m_ResolvedRoughnessTex = AssetManager::Get().GetAsset<TextureAsset>(roughnessTextureUUID);
    return m_ResolvedRoughnessTex;
}

Ref<TextureAsset> MaterialAsset::GetAOTexture() const
{
    if (!useAOMap || !aoTextureUUID.IsValid()) return nullptr;
    if (!m_ResolvedAOTex || m_ResolvedAOTex->GetUUID() != aoTextureUUID)
        m_ResolvedAOTex = AssetManager::Get().GetAsset<TextureAsset>(aoTextureUUID);
    return m_ResolvedAOTex;
}

Ref<TextureAsset> MaterialAsset::GetHeightTexture() const
{
    if (!useHeightMap || !heightTextureUUID.IsValid()) return nullptr;
    if (!m_ResolvedHeightTex || m_ResolvedHeightTex->GetUUID() != heightTextureUUID)
        m_ResolvedHeightTex = AssetManager::Get().GetAsset<TextureAsset>(heightTextureUUID);
    return m_ResolvedHeightTex;
}

Ref<TextureAsset> MaterialAsset::GetARMTexture() const
{
    if (!useARMMap || !armTextureUUID.IsValid()) return nullptr;
    if (!m_ResolvedARMTex || m_ResolvedARMTex->GetUUID() != armTextureUUID)
        m_ResolvedARMTex = AssetManager::Get().GetAsset<TextureAsset>(armTextureUUID);
    return m_ResolvedARMTex;
}

Ref<ShaderAsset> MaterialAsset::GetCustomShaderAsset() const
{
    if (!customShaderUUID.IsValid()) return nullptr;
    if (!m_ResolvedCustomShader || m_ResolvedCustomShader->GetUUID() != customShaderUUID)
        m_ResolvedCustomShader = AssetManager::Get().GetAsset<ShaderAsset>(customShaderUUID);
    return m_ResolvedCustomShader;
}

MaterialOverrideData& MaterialInstance::EnsureOverrides()
{
    if (!m_Overrides)
        m_Overrides = std::make_unique<MaterialOverrideData>();
    return *m_Overrides;
}

void MaterialInstance::SetOverride(MatField f)
{
    EnsureOverrides().overrideMask.set(static_cast<size_t>(f));
}

void MaterialInstance::ClearOverride(MatField f)
{
    if (m_Overrides)
    {
        m_Overrides->overrideMask.reset(static_cast<size_t>(f));
        if (m_Overrides->overrideMask.none() && (!m_Overrides->customUniformOverrides || m_Overrides->customUniformOverrides->empty()))
        {
            m_Overrides.reset();
        }
    }
}

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
    if (!m_Overrides) return j;

    const auto& o = *m_Overrides;

    if (IsOverridden(MatField::AlbedoColor))
        j["albedoColor"] = { o.albedoColor.r, o.albedoColor.g, o.albedoColor.b, o.albedoColor.a };
    if (IsOverridden(MatField::EmissiveColor))
        j["emissiveColor"] = { o.emissiveColor.r, o.emissiveColor.g, o.emissiveColor.b };
    if (IsOverridden(MatField::EmissiveIntensity))
        j["emissiveIntensity"] = o.emissiveIntensity;
    if (IsOverridden(MatField::Metallic))
        j["metallic"] = o.metallic;
    if (IsOverridden(MatField::Specular))
        j["specular"] = o.specular;
    if (IsOverridden(MatField::Roughness))
        j["roughness"] = o.roughness;
    if (IsOverridden(MatField::AO))
        j["ao"] = o.ao;
    if (IsOverridden(MatField::NormalScale))
        j["normalScale"] = o.normalScale;
    if (IsOverridden(MatField::HeightScale))
        j["heightScale"] = o.heightScale;
    if (IsOverridden(MatField::InvertHeight))
        j["invertHeight"] = o.invertHeight;
    if (IsOverridden(MatField::UVScale))
        j["UVScale"] = { o.UVScale.x, o.UVScale.y };
    if (IsOverridden(MatField::UVOffset))
        j["UVOffset"] = { o.UVOffset.x, o.UVOffset.y };

    if (IsOverridden(MatField::TransparencyMode))
        j["transparencyMode"] = static_cast<int>(o.transparencyMode);
    if (IsOverridden(MatField::BlendMode))
        j["blendMode"] = static_cast<int>(o.blendMode);
    if (IsOverridden(MatField::CullMode))
        j["cullMode"] = static_cast<int>(o.cullMode);
    if (IsOverridden(MatField::AlphaCutoff))
        j["alphaCutoff"] = o.alphaCutoff;

    if (IsOverridden(MatField::BillboardMode))
        j["billboardMode"] = static_cast<int>(o.billboardMode);

    if (IsOverridden(MatField::ShadingMode))
        j["shadingMode"] = static_cast<int>(o.shadingMode);
    if (IsOverridden(MatField::ReceiveShadows))
        j["receiveShadows"] = o.receiveShadows;
    if (IsOverridden(MatField::UseTriplanar))
        j["useTriplanar"] = o.useTriplanar;
    if (IsOverridden(MatField::UseWorldTriplanar))
        j["useWorldTriplanar"] = o.useWorldTriplanar;
    if (IsOverridden(MatField::TriplanarSharpness))
        j["triplanarSharpness"] = o.triplanarSharpness;

    if (IsOverridden(MatField::HasOutline))
        j["hasOutline"] = o.hasOutline;
    if (IsOverridden(MatField::OutlineColor))
        j["outlineColor"] = { o.outlineColor.r, o.outlineColor.g, o.outlineColor.b, o.outlineColor.a };
    if (IsOverridden(MatField::OutlineWidth))
        j["outlineWidth"] = o.outlineWidth;
    if (IsOverridden(MatField::OutlineFixedSize))
        j["outlineFixedSize"] = o.outlineFixedSize;
    if (IsOverridden(MatField::OutlineXRay))
        j["outlineXRay"] = o.outlineXRay;

    if (IsOverridden(MatField::AlbedoTex))
        j["albedoTex"] = o.albedoTex ? o.albedoTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::NormalTex))
        j["normalTex"] = o.normalTex ? o.normalTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::MetallicTex))
        j["metallicTex"] = o.metallicTex ? o.metallicTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::RoughnessTex))
        j["roughnessTex"] = o.roughnessTex ? o.roughnessTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::AOTex))
        j["aoTex"] = o.aoTex ? o.aoTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::HeightTex))
        j["heightTex"] = o.heightTex ? o.heightTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::ARMTex))
        j["armTex"] = o.armTex ? o.armTex->GetUUID().ToString() : "";
    if (IsOverridden(MatField::UseARMMap))
        j["useARMMap"] = o.useARMMap;

    if (IsOverridden(MatField::CustomShader))
        j["customShader"] = o.customShaderUUID.ToString();

    if (IsOverridden(MatField::TextureFilterMode))
        j["textureFilter"] = static_cast<int>(o.textureFilter);
    if (IsOverridden(MatField::TextureWrapMode))
        j["textureWrap"] = static_cast<int>(o.textureWrap);
    if (IsOverridden(MatField::TextureAnisotropyMode))
        j["textureAnisotropy"] = static_cast<int>(o.textureAnisotropy);

    if (o.customUniformOverrides && !o.customUniformOverrides->empty())
    {
        json uJson = json::object();
        for (const auto& [name, val] : *o.customUniformOverrides)
        {
            json item;
            item["type"] = static_cast<int>(val.type);
            switch (val.type)
            {
            case ShaderUniformType::Float: item["value"] = val.floatVal; break;
            case ShaderUniformType::Int: item["value"] = val.intVal; break;
            case ShaderUniformType::Bool: item["value"] = val.boolVal; break;
            case ShaderUniformType::Vec2: item["value"] = { val.vec2Val.x, val.vec2Val.y }; break;
            case ShaderUniformType::Vec3:
            case ShaderUniformType::Color3: item["value"] = { val.vec3Val.x, val.vec3Val.y, val.vec3Val.z }; break;
            case ShaderUniformType::Vec4:
            case ShaderUniformType::Color4: item["value"] = { val.vec4Val.x, val.vec4Val.y, val.vec4Val.z, val.vec4Val.w }; break;
            case ShaderUniformType::Texture2D: item["value"] = val.textureUUID.ToString(); break;
            }
            uJson[name] = item;
        }
        j["customUniformOverrides"] = uJson;
    }

    return j;
}

void MaterialInstance::DeserializeOverrides(const json& j)
{
    if (j.empty() || !j.is_object()) return;
    auto& mgr = AssetManager::Get();

    if (j.contains("albedoColor")) {
        auto& col = j["albedoColor"];
        SetAlbedoColor({ col[0], col[1], col[2], col.size() > 3 ? col[3].get<float>() : 1.0f });
    }
    if (j.contains("emissiveColor"))
    {
        SetEmissiveColor({ j["emissiveColor"][0], j["emissiveColor"][1], j["emissiveColor"][2] });
    }
    if (j.contains("emissiveIntensity"))
    {
        SetEmissiveIntensity(j["emissiveIntensity"]);
    }
    if (j.contains("metallic"))
    {
        SetMetallic(j["metallic"]);
    }
    if (j.contains("specular"))
    {
        SetSpecular(j["specular"]);
    }
    if (j.contains("roughness"))
    {
        SetRoughness(j["roughness"]);
    }
    if (j.contains("ao"))
    {
        SetAO(j["ao"]);
    }
    if (j.contains("normalScale"))
    {
        SetNormalScale(j["normalScale"]);
    }
    if (j.contains("heightScale"))
    {
        SetHeightScale(j["heightScale"]);
    }
    if (j.contains("invertHeight"))
    {
        SetInvertHeight(j["invertHeight"].get<bool>());
    }
    if (j.contains("UVScale"))
    {
        auto& uv = j["UVScale"];
        SetUVScale({ uv[0], uv[1] });
    }
    if (j.contains("UVOffset"))
    {
        auto& uv = j["UVOffset"];
        SetUVOffset({ uv[0], uv[1] });
    }

    if (j.contains("useTriplanar"))
    {
        SetUseTriplanar(j["useTriplanar"].get<bool>());
    }
    if (j.contains("useWorldTriplanar"))
    {
        SetUseWorldTriplanar(j["useWorldTriplanar"].get<bool>());
    }
    if (j.contains("triplanarSharpness"))
    {
        SetTriplanarSharpness(j["triplanarSharpness"].get<float>());
    }

    if (j.contains("transparencyMode"))
    {
        SetTransparencyMode(static_cast<TransparencyMode>(j["transparencyMode"].get<int>()));
    }
    if (j.contains("blendMode"))
    {
        SetBlendMode(static_cast<BlendMode>(j["blendMode"].get<int>()));
    }
    if (j.contains("cullMode"))
    {
        SetCullMode(static_cast<CullMode>(j["cullMode"].get<int>()));
    }
    if (j.contains("alphaCutoff"))
    {
        SetAlphaCutoff(j["alphaCutoff"].get<float>());
    }

    if (j.contains("billboardMode"))
    {
        SetBillboardMode(static_cast<BillboardMode>(j["billboardMode"].get<int>()));
    }

    if (j.contains("shadingMode")) {
        SetShadingMode(static_cast<ShadingMode>(j["shadingMode"].get<int>()));
    }
    if (j.contains("receiveShadows")) {
        SetReceiveShadows(j["receiveShadows"].get<bool>());
    }

    if (j.contains("hasOutline"))
    {
        SetHasOutline(j["hasOutline"].get<bool>());
    }
    if (j.contains("outlineColor"))
    {
        auto& c = j["outlineColor"];
        SetOutlineColor({ c[0], c[1], c[2], c.size() > 3 ? c[3].get<float>() : 1.0f });
    }
    if (j.contains("outlineWidth"))
    {
        SetOutlineWidth(j["outlineWidth"].get<float>());
    }
    if (j.contains("outlineFixedSize"))
    {
        SetOutlineFixedSize(j["outlineFixedSize"].get<bool>());
    }
    if (j.contains("outlineXRay"))
    {
        SetOutlineXRay(j["outlineXRay"].get<bool>());
    }

    auto loadTex = [&](const char* key, auto setterFn)
    {
        if (!j.contains(key)) return;
        std::string uuidStr = j[key].get<std::string>();
        if (!uuidStr.empty())
        {
            auto uuid = AssetUUID::FromString(uuidStr);
            setterFn(mgr.GetAsset<TextureAsset>(uuid));
        }
        else
        {
            setterFn(Ref<TextureAsset>(nullptr));
        }
    };

    loadTex("albedoTex", [&](Ref<TextureAsset> tex) { SetAlbedoTexture(tex); });
    loadTex("normalTex", [&](Ref<TextureAsset> tex) { SetNormalTexture(tex); });
    loadTex("metallicTex", [&](Ref<TextureAsset> tex) { SetMetallicTexture(tex); });
    loadTex("roughnessTex", [&](Ref<TextureAsset> tex) { SetRoughnessTexture(tex); });
    loadTex("aoTex", [&](Ref<TextureAsset> tex) { SetAOTexture(tex); });
    loadTex("heightTex", [&](Ref<TextureAsset> tex) { SetHeightTexture(tex); });
    loadTex("armTex", [&](Ref<TextureAsset> tex) { SetARMTexture(tex); });
    if (j.contains("useARMMap"))
    {
        SetUseARMMap(j["useARMMap"].get<bool>());
    }

    if (j.contains("customShader"))
    {
        SetCustomShader(AssetUUID::FromString(j["customShader"].get<std::string>()));
    }

    if (j.contains("textureFilter"))
        SetTextureFilter(static_cast<TextureFilter>(j["textureFilter"].get<int>()));
    if (j.contains("textureWrap"))
        SetTextureWrap(static_cast<TextureWrap>(j["textureWrap"].get<int>()));
    if (j.contains("textureAnisotropy"))
        SetTextureAnisotropy(static_cast<TextureAnisotropy>(j["textureAnisotropy"].get<int>()));

    if (j.contains("customUniformOverrides") && j["customUniformOverrides"].is_object())
    {
        for (auto& [name, item] : j["customUniformOverrides"].items())
        {
            CustomUniformValue val;
            val.type = static_cast<ShaderUniformType>(item.value("type", 0));
            auto& v = item["value"];
            switch (val.type)
            {
            case ShaderUniformType::Float: val.floatVal = v.get<float>(); break;
            case ShaderUniformType::Int: val.intVal = v.get<int>(); break;
            case ShaderUniformType::Bool: val.boolVal = v.get<bool>(); break;
            case ShaderUniformType::Vec2: val.vec2Val = glm::vec2(v[0].get<float>(), v[1].get<float>()); break;
            case ShaderUniformType::Vec3:
            case ShaderUniformType::Color3: val.vec3Val = glm::vec3(v[0].get<float>(), v[1].get<float>(), v[2].get<float>()); break;
            case ShaderUniformType::Vec4:
            case ShaderUniformType::Color4: val.vec4Val = glm::vec4(v[0].get<float>(), v[1].get<float>(), v[2].get<float>(), v[3].get<float>()); break;
            case ShaderUniformType::Texture2D: val.textureUUID = AssetUUID::FromString(v.get<std::string>()); break;
            }
            SetCustomUniform(name, val);
        }
    }
}

void MaterialInstance::SetCustomShader(const AssetUUID& uuid)
{
    auto& o = EnsureOverrides();
    o.customShaderUUID = uuid;
    o.customShader = uuid.IsValid() ? AssetManager::Get().GetAsset<ShaderAsset>(uuid) : nullptr;
    SetOverride(MatField::CustomShader);
}

void MaterialInstance::ClearCustomShader()
{
    if (m_Overrides)
    {
        m_Overrides->customShaderUUID = AssetUUID::Invalid();
        m_Overrides->customShader = nullptr;
    }
    ClearOverride(MatField::CustomShader);
}

void MaterialInstance::RebuildCache()
{
    if (!m_SourceAsset) return;
    m_SourceAsset->InvalidateTextureCache();

    m_Sampler.GetDesc().filter = GetTextureFilter();
    m_Sampler.GetDesc().repeat = GetTextureWrap();
    m_Sampler.GetDesc().aniso = GetTextureAnisotropy();
    m_Sampler.GetDesc().minFilter = m_SourceAsset->samplerDesc.minFilter;
    m_Sampler.GetDesc().magFilter = m_SourceAsset->samplerDesc.magFilter;
    m_Sampler.GetDesc().wrap = m_SourceAsset->samplerDesc.wrap;
    m_Sampler.GetDesc().anisotropy = m_SourceAsset->samplerDesc.anisotropy;
}

CustomUniformValue MaterialInstance::GetCustomUniform(const std::string& name) const
{
    if (m_Overrides && m_Overrides->customUniformOverrides)
    {
        auto itOver = m_Overrides->customUniformOverrides->find(name);
        if (itOver != m_Overrides->customUniformOverrides->end())
            return itOver->second;
    }

    if (m_SourceAsset)
    {
        auto itCached = m_SourceAsset->customUniforms.find(name);
        if (itCached != m_SourceAsset->customUniforms.end())
            return itCached->second;
    }

    auto shaderAsset = GetCustomShaderAsset();
    if (shaderAsset)
    {
        const auto* def = shaderAsset->FindUniform(name);
        if (def)
        {
            CustomUniformValue val;
            val.type = def->type;
            val.floatVal = def->floatVal;
            val.intVal = def->intVal;
            val.boolVal = def->boolVal;
            val.vec2Val = def->vec2Val;
            val.vec3Val = def->vec3Val;
            val.vec4Val = def->vec4Val;
            val.textureUUID = def->textureUUID;
            return val;
        }
    }

    return CustomUniformValue{};
}

void MaterialInstance::SetCustomUniform(const std::string& name, const CustomUniformValue& val)
{
    auto& o = EnsureOverrides();
    if (!o.customUniformOverrides)
        o.customUniformOverrides = std::make_unique<std::unordered_map<std::string, CustomUniformValue>>();
    (*o.customUniformOverrides)[name] = val;
}

void MaterialInstance::ClearCustomUniform(const std::string& name)
{
    if (m_Overrides && m_Overrides->customUniformOverrides)
    {
        m_Overrides->customUniformOverrides->erase(name);
        if (m_Overrides->customUniformOverrides->empty())
        {
            m_Overrides->customUniformOverrides.reset();
            if (m_Overrides->overrideMask.none())
                m_Overrides.reset();
        }
    }
}

bool MaterialInstance::IsCustomUniformOverridden(const std::string& name) const
{
    if (!m_Overrides || !m_Overrides->customUniformOverrides) return false;
    return m_Overrides->customUniformOverrides->find(name) != m_Overrides->customUniformOverrides->end();
}

void MaterialInstance::ClearAllCustomUniformOverrides()
{
    if (m_Overrides && m_Overrides->customUniformOverrides)
    {
        m_Overrides->customUniformOverrides.reset();
        if (m_Overrides->overrideMask.none())
            m_Overrides.reset();
    }
}
