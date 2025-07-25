#pragma once
#include "Assets/Asset.h"
#include "glm/glm.hpp"
#include "Core/Ref.h"
#include "Assets/TextureAsset.h"
#include "AssetMeta.h"
#include "Assets/AssetUUID.h"

struct MaterialMeta : public AssetMeta
{

};

class MaterialAsset : public Asset
{
public:
	inline glm::vec3 GetAlbedoColor() const noexcept { return albedoColor; }
	inline void SetAlbedoColor(const glm::vec3& albedoColor) noexcept { this->albedoColor = albedoColor; }
	inline float GetMetallic() const noexcept { return metallic; }
	inline void SetMetallic(float metallic) noexcept { this->metallic = metallic; }
	inline float GetRoughness() const noexcept { return roughness; }
	inline void SetRoughness(float roughness) noexcept { this->roughness = roughness; }
	inline float GetAO() const noexcept { return ao; }
	inline void SetAO(float ao) noexcept { this->ao = ao; }
	inline float GetNormalScale() const noexcept { return normalScale; }
	inline void SetNormalScale() noexcept { this->normalScale = normalScale; }
	inline glm::vec2 GetUVScale() const noexcept { return UVScale; }
	inline void SetUVScale(const glm::vec2& UVScale) noexcept { this->UVScale = UVScale; }
	inline glm::vec2 GetUVOffset() const noexcept { return UVOffset; }
	inline void SetUVOffset(const glm::vec2& UVOffset) noexcept { this->UVOffset = UVOffset; }


private:
	glm::vec3 albedoColor = glm::vec3(1.0f);
	float metallic = 0.0f;
	float roughness = 1.0f;
	float ao = 1.0f;
	float normalScale = 1.0f;
	glm::vec2 UVScale = glm::vec2(1.0f);
	glm::vec2 UVOffset = glm::vec2(0.0f);

	Ref<TextureAsset> albedoTextureUUID;
	Ref<TextureAsset> normalTextureUUID;
	Ref<TextureAsset> metallicTextureUUID;
	Ref<TextureAsset> roughnessTextureUUID;
	Ref<TextureAsset> aoTextureUUID;
	Ref<TextureAsset> heightTextureUUID;

};


struct MatComponent
{
	Ref<MaterialAsset> material;
};