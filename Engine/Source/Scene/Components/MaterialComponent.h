#pragma once
#include "Utils/FileUtils.h"
#include "json.hpp"
#include "Core/Ref.h"
#include "Assets/AssetRegistry.h"
#include "Assets/TextureAsset.h"
#include "Assets/MaterialAsset.h"

namespace rv {

using json = nlohmann::json;

struct MaterialComponent
{
public:
	MaterialComponent() {}
	MaterialComponent(const AssetUUID& uuid) 
	{
		Load(uuid);
	}

	inline glm::vec3 GetAlbedoColor() const noexcept { return material->albedoColor; }
	inline void SetAlbedoColor(const glm::vec3& albedoColor) noexcept { material->albedoColor = albedoColor; }
	inline glm::vec3 GetEmmisiveColor() const noexcept { return material->emmisiveColor; }
	inline void SetEmmisiveColor(const glm::vec3& emmisiveColor) noexcept { material->emmisiveColor = emmisiveColor; }
	inline float GetEmmisiveIntensity() const noexcept { return material->emmisiveIntensity; }
	inline void SetEmmisiveIntensity(float emmisiveIntensity) const noexcept { material->emmisiveIntensity = emmisiveIntensity; }
	inline float GetMetallic() const noexcept { return material->metallic; }
	inline void SetMetallic(float metallic) noexcept { material->metallic = metallic; }
	inline float GetSpecular() const noexcept { return material->specular; }
	inline void SetSpecular(float specular) noexcept { material->specular = specular; }
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

	inline bool IsUsingAlbedoMap() const noexcept { return material->useAlbedoMap; }
	inline void SetUseAlbedoMap(bool value) noexcept { material->useAlbedoMap = value; }
	inline bool IsUsingNormalMap() const noexcept { return material->useNormalMap; }
	inline void SetUseNormalMap(bool value) noexcept { material->useNormalMap = value; }
	inline bool IsUsingMetallicMap() const noexcept { return material->useMetallicMap; }
	inline void SetUseMetallicMap(bool value) noexcept { material->useMetallicMap = value; }
	inline bool IsUsingRoughnessMap() const noexcept { return material->useRoughnessMap; }
	inline void SetUseRoughnessMap(bool value) noexcept { material->useRoughnessMap = value; }
	inline bool IsUsingAOMap() const noexcept { return material->useAOMap; }
	inline void SetUseAOMap(bool value) noexcept { material->useAOMap = value; }
	inline bool IsUsingHeightMap() const noexcept { return material->useHeightMap; }
	inline void SetUseHeightMap(bool value) noexcept { material->useHeightMap = value; }

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

	Sampler& GetSampler() { return material->sampler; }
	void SetSampler(const SamplerDesc& samplerDescc) { material->sampler.Init(samplerDescc); }

	inline const AssetUUID& GetMaterialID() const noexcept{ return materialUUID; }

	json Serialize() const;
	void Deserialize(const json& j);

private:

	void Load(const AssetUUID& uuid);

	Ref<TextureAsset> albedoTexture;
	Ref<TextureAsset> normalTexture;
	Ref<TextureAsset> metallicTexture;
	Ref<TextureAsset> roughnessTexture;
	Ref<TextureAsset> aoTexture;
	Ref<TextureAsset> heightTexture;
	Ref<MaterialAsset> material;
	AssetUUID materialUUID;

};

}