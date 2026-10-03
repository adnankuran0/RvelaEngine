#include "rvelapch.h"
#include "MaterialComponent.h"
#include "Asset/AssetManager.h"
#include "Core/Log.h"

using namespace rv;

using json = nlohmann::json;

MaterialComponent::MaterialComponent()
    : m_MaterialUUID(AssetUUID::Invalid()), m_Instance()
{
}

MaterialComponent::MaterialComponent(const AssetUUID& uuid)
    : m_MaterialUUID(AssetUUID::Invalid()), m_Instance()
{
    Load(uuid);
}

void MaterialComponent::Load(const AssetUUID& uuid)
{
    if (!uuid.IsValid() || uuid == s_LegacyDefaultMaterialUUID)
    {
        m_MaterialUUID = AssetUUID::Invalid();
        m_Instance = MaterialInstance{};
        return;
    }

    m_MaterialUUID = uuid;

    Ref<MaterialAsset> asset = AssetManager::Get().GetAsset<MaterialAsset>(uuid);
    if (!asset)
    {
        m_MaterialUUID = AssetUUID::Invalid();
        m_Instance = MaterialInstance{};
        return;
    }

    m_Instance = MaterialInstance::CreateFromAsset(asset);
}

json MaterialComponent::Serialize() const
{
    json j;
    if (m_MaterialUUID.IsValid() && m_MaterialUUID != s_LegacyDefaultMaterialUUID)
        j["material"] = m_MaterialUUID.ToString();
    else
        j["material"] = "";

    if (m_Instance.HasAnyOverride())
        j["overrides"] = m_Instance.SerializeOverrides();

    return j;
}

void MaterialComponent::Deserialize(const json& j)
{
    if (j.contains("material"))
    {
        std::string uuidStr = j.at("material").get<std::string>();
        if (uuidStr.empty() || uuidStr == "00000000-0000-0000-0000-000000000000")
        {
            Load(AssetUUID::Invalid());
        }
        else
        {
            AssetUUID uuid = AssetUUID::FromString(uuidStr);
            Load(uuid);
        }
    }
    else
    {
        Load(AssetUUID::Invalid());
    }

    if (j.contains("overrides"))
        m_Instance.DeserializeOverrides(j["overrides"]);
}