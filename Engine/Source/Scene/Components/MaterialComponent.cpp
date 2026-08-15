#include "rvelapch.h"
#include "MaterialComponent.h"
#include "Asset/AssetManager.h"
#include "Core/Log.h"

using namespace rv;

using json = nlohmann::json;

void MaterialComponent::Load(const AssetUUID& uuid)
{
    if (!uuid.IsValid())
    {
        LOG_WARN("Invalid UUID");
        return;
    }

    m_MaterialUUID = uuid;

    Ref<MaterialAsset> asset = AssetManager::Get().GetAsset<MaterialAsset>(uuid);
    if (!asset)
    {
        // LOG_ERROR("MaterialAsset not found: {}", uuid.ToString());
        // TODO: DEFAULT MATERIAL DOESNT NEED ANY UUID!!!
        return;
    }

    m_Instance = MaterialInstance::CreateFromAsset(asset);
}

json MaterialComponent::Serialize() const
{
    json j;
    j["material"] = m_MaterialUUID.ToString();

    if (m_Instance.HasAnyOverride())
        j["overrides"] = m_Instance.SerializeOverrides();

    return j;
}

void MaterialComponent::Deserialize(const json& j)
{
    AssetUUID uuid = AssetUUID::FromString(j.at("material").get<std::string>());
    Load(uuid);

    if (j.contains("overrides"))
        m_Instance.DeserializeOverrides(j["overrides"]);
}