#include "rvelapch.h"
#include "MaterialComponent.h"

void MaterialComponent::Load(const AssetUUID& uuid)
{
	if (!uuid.IsValid())
	{
		LOG_WARN("Material UUID is not valid!");
		return;
	}
	materialUUID = uuid;

	// Clear existing
	material.Reset();
	albedoTexture.Reset();
	normalTexture.Reset();
	metallicTexture.Reset();
	roughnessTexture.Reset();
	aoTexture.Reset();
	heightTexture.Reset();

	material = AssetRegistry::GetAsset<MaterialAsset>(materialUUID);
	if (!material)
	{
		LOG_ERROR("Material asset not found for UUID {}", materialUUID.ToString());
		return;
	}
	if (material->useAlbedoMap)
		albedoTexture = AssetRegistry::GetAsset<TextureAsset>(material->albedoTextureUUID);
	if (material->useNormalMap)
		normalTexture = AssetRegistry::GetAsset<TextureAsset>(material->normalTextureUUID);
	if (material->useMetallicMap)
		metallicTexture = AssetRegistry::GetAsset<TextureAsset>(material->metallicTextureUUID);
	if (material->useRoughnessMap)
		roughnessTexture = AssetRegistry::GetAsset<TextureAsset>(material->roughnessTextureUUID);
	if (material->useAOMap)
		aoTexture = AssetRegistry::GetAsset<TextureAsset>(material->aoTextureUUID);
	if (material->useHeightMap)
		heightTexture = AssetRegistry::GetAsset<TextureAsset>(material->heightTextureUUID);

}

json MaterialComponent::Serialize() const
{
	json j;
	j["material"] = materialUUID.ToString();
	material->Serialize();
	return j;
}
void MaterialComponent::Deserialize(const json& j)
{
	std::string materialUUIDstr = j.at("material").get<std::string>();
	materialUUID = AssetUUID::FromString(materialUUIDstr);
	Load(materialUUID);

}