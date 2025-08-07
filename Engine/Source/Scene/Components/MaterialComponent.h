#pragma once
#include "Scene/Component.h"
#include "Utils/FileUtils.h"
#include "json.hpp"
#include "Core/Ref.h"
#include "Assets/AssetRegistry.h"


using json = nlohmann::json;

class MaterialComponent : public Component
{
public:
	MaterialComponent() {}
	MaterialComponent(const AssetUUID& uuid) 
	{
		Load(uuid);
	}

	inline glm::vec3 GetAlbedoColor() const noexcept { return material->albedoColor; }
	inline void SetAlbedoColor(const glm::vec3& albedoColor) noexcept { material->albedoColor = albedoColor; }
	inline float GetMetallic() const noexcept { return material->metallic; }
	inline void SetMetallic(float metallic) noexcept { material->metallic = metallic; }
	inline float GetRoughness() const noexcept { return material->roughness; }
	inline void SetRoughness(float roughness) noexcept { material->roughness = roughness; }
	inline float GetAO() const noexcept { return material->ao; }
	inline void SetAO(float ao) noexcept { material->ao = ao; }
	inline float GetNormalScale() const noexcept { return material->normalScale; }
	inline void SetNormalScale(float normalScale) noexcept { material->normalScale = normalScale; }
	inline glm::vec2 GetUVScale() const noexcept { return material->UVScale; }
	inline void SetUVScale(const glm::vec2& UVScale) noexcept { material->UVScale = UVScale; }
	inline glm::vec2 GetUVOffset() const noexcept { return material->UVOffset; }
	inline void SetUVOffset(const glm::vec2& UVOffset) noexcept { material->UVOffset = UVOffset; }

	bool IsUsingAlbedoMap() const noexcept { return material->useAlbedoMap; }
	void SetUseAlbedoMap(bool value) noexcept { material->useAlbedoMap = value; }
	bool IsUsingNormalMap() const noexcept { return material->useNormalMap; }
	void SetUseNormalMap(bool value) noexcept { material->useNormalMap = value; }
	bool IsUsingMetallicMap() const noexcept { return material->useMetallicMap; }
	void SetUseMetallicMap(bool value) noexcept { material->useMetallicMap = value; }
	bool IsUsingRoughnessMap() const noexcept { return material->useRoughnessMap; }
	void SetUseRoughnessMap(bool value) noexcept { material->useRoughnessMap = value; }
	bool IsUsingAOMap() const noexcept { return material->useAOMap; }
	void SetUseAOMap(bool value) noexcept { material->useAOMap = value; }
	bool IsUsingHeightMap() const noexcept { return material->useHeightMap; }
	void SetUseHeightMap(bool value) noexcept { material->useHeightMap = value; }

	inline Ref<TextureAsset> GetAlbedoTexture() const { return albedoTexture; }
	inline void SetAlbedoTexture(const AssetUUID& textureUIID)
	{
		material->albedoTextureUUID = textureUIID;
		albedoTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	inline Ref<TextureAsset> GetNormalTexture() const { return normalTexture; }
	inline void SetNormalTexture(const AssetUUID& textureUIID)
	{
		material->normalTextureUUID = textureUIID;
		normalTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	inline Ref<TextureAsset> GetMetallicTexture() const { return metallicTexture; }
	inline void SetMetallicTexture(const AssetUUID& textureUIID)
	{
		material->metallicTextureUUID = textureUIID;
		metallicTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	inline Ref<TextureAsset> GetRoughnessTexture() const { return roughnessTexture; }
	inline void SetRoughnessTexture(const AssetUUID& textureUIID)
	{
		material->roughnessTextureUUID = textureUIID;
		roughnessTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	inline Ref<TextureAsset> GetAOTexture() const { return aoTexture; }
	inline void SetAOTexture(const AssetUUID& textureUIID)
	{
		material->aoTextureUUID = textureUIID;
		aoTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	inline Ref<TextureAsset> GetHeightTexture() const { return heightTexture; }
	inline void SetHeightTexture(const AssetUUID& textureUIID)
	{
		material->heightTextureUUID = textureUIID;
		heightTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	Ref<MaterialAsset> GetMaterial() { return material; }
	void SetMaterial(AssetUUID newMaterialUUID)
	{
		Load(newMaterialUUID);
	}

	std::string Serialize() const override
	{
		json j;
		j["material"] = materialUUID.ToString();
		material->Serialize();
		return j.dump(4);
	}
	void Deserialize(const std::string& jsonStr) override
	{
		json j = json::parse(jsonStr);
		std::string materialUUIDstr = j["material"];
		materialUUID = AssetUUID::FromString(materialUUIDstr);
		Load(materialUUID);

	}

private:

	void Load(const AssetUUID& uuid)
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
		if(material->useAlbedoMap)
			albedoTexture = AssetRegistry::GetAsset<TextureAsset>(material->albedoTextureUUID);
		if(material->useNormalMap)
			normalTexture = AssetRegistry::GetAsset<TextureAsset>(material->normalTextureUUID);
		if(material->useMetallicMap)
			metallicTexture = AssetRegistry::GetAsset<TextureAsset>(material->metallicTextureUUID);
		if(material->useRoughnessMap)
			roughnessTexture = AssetRegistry::GetAsset<TextureAsset>(material->roughnessTextureUUID);
		if(material->useAOMap)
			aoTexture = AssetRegistry::GetAsset<TextureAsset>(material->aoTextureUUID);
		if (material->useHeightMap)
			heightTexture = AssetRegistry::GetAsset<TextureAsset>(material->heightTextureUUID);
		
	}

	Ref<TextureAsset> albedoTexture;
	Ref<TextureAsset> normalTexture;
	Ref<TextureAsset> metallicTexture;
	Ref<TextureAsset> roughnessTexture;
	Ref<TextureAsset> aoTexture;
	Ref<TextureAsset> heightTexture;

	Ref<MaterialAsset> material;
	AssetUUID materialUUID;

};

