#pragma once
#include "Assets/Asset.h"
#include "glm/glm.hpp"
#include "Core/Ref.h"
#include "Assets/TextureAsset.h"
#include "AssetMeta.h"
#include "Assets/AssetUUID.h"
#include <memory>

#if 0
struct MaterialMeta : public AssetMeta
{
	std::unique_ptr<AssetMeta> Clone() const override
	{
		return std::make_unique<MaterialMeta>(*this);
	}

	void Serialize(std::vector<char>& outBuffer, size_t& offset) const override {
		SerializeBase(outBuffer, offset);
	}

	void Deserialize(const std::vector<char>& inBuffer, size_t& offset) override {
		DeserializeBase(inBuffer, offset);
	}
};

class MaterialAsset : public Asset
{
public:
	MaterialAsset(const std::string& path, std::unique_ptr<MaterialMeta> meta)
		: Asset(std::move(meta)), m_Path(path) {}

	inline glm::vec3 GetAlbedoColor() const noexcept { return albedoColor; }
	inline void SetAlbedoColor(const glm::vec3& albedoColor) noexcept { this->albedoColor = albedoColor; }
	inline float GetMetallic() const noexcept { return metallic; }
	inline void SetMetallic(float metallic) noexcept { this->metallic = metallic; }
	inline float GetRoughness() const noexcept { return roughness; }
	inline void SetRoughness(float roughness) noexcept { this->roughness = roughness; }
	inline float GetAO() const noexcept { return ao; }
	inline void SetAO(float ao) noexcept { this->ao = ao; }
	inline float GetNormalScale() const noexcept { return normalScale; }
	inline void SetNormalScale(float normalScale) noexcept { this->normalScale = normalScale; }
	inline glm::vec2 GetUVScale() const noexcept { return UVScale; }
	inline void SetUVScale(const glm::vec2& UVScale) noexcept { this->UVScale = UVScale; }
	inline glm::vec2 GetUVOffset() const noexcept { return UVOffset; }
	inline void SetUVOffset(const glm::vec2& UVOffset) noexcept { this->UVOffset = UVOffset; }

	inline Ref<TextureAsset> GetAlbedoTexture() const { return albedoTexture; }
	inline void SetAlbedoTexture(const AssetUUID& textureUIID) 
	{
		albedoTextureUUID = textureUIID.IsValid() ? textureUIID : AssetUUID();
		albedoTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	inline Ref<TextureAsset> GetNormalTexture() const { return normalTexture; }
	inline void SetNormalTexture(const AssetUUID& textureUIID)
	{
		normalTextureUUID = textureUIID.IsValid() ? textureUIID : AssetUUID();
		normalTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}	

	inline Ref<TextureAsset> GetMetallicTexture() const { return metallicTexture; }
	inline void SetMetallicTexture(const AssetUUID& textureUIID)
	{
		metallicTextureUUID = textureUIID.IsValid() ? textureUIID : AssetUUID();
		metallicTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}	

	inline Ref<TextureAsset> GetRoughnessTexture() const { return roughnessTexture; }
	inline void SetRoughnessTexture(const AssetUUID& textureUIID)
	{
		roughnessTextureUUID = textureUIID.IsValid() ? textureUIID : AssetUUID();
		roughnessTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}	

	inline Ref<TextureAsset> GetAOTexture() const { return aoTexture; }
	inline void SetAOTexture(const AssetUUID& textureUIID)
	{
		aoTextureUUID = textureUIID.IsValid() ? textureUIID : AssetUUID();
		aoTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	inline Ref<TextureAsset> GetHeightTexture() const { return heightTexture; }
	inline void SetHeightTexture(const AssetUUID& textureUIID)
	{
		heightTextureUUID = textureUIID.IsValid() ? textureUIID : AssetUUID();
		heightTexture = AssetRegistry::GetAsset<TextureAsset>(textureUIID);
	}

	inline bool Load() 
	{
		std::ifstream file(m_Path, std::ios::binary);
		if (!file) {
			LOG_ERROR("Failed to open .rmat: {}", m_Path);
			return false;
		}

		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), {});
		file.close();

		size_t offset = 0;
		GetMetaAs<MaterialMeta>()->Deserialize(buffer, offset);

		memcpy(&albedoColor, buffer.data() + offset, sizeof(glm::vec3)); offset += sizeof(glm::vec3);
		memcpy(&metallic, buffer.data() + offset, sizeof(float)); offset += sizeof(float);
		memcpy(&roughness, buffer.data() + offset, sizeof(float)); offset += sizeof(float);
		memcpy(&ao, buffer.data() + offset, sizeof(float)); offset += sizeof(float);
		memcpy(&normalScale, buffer.data() + offset, sizeof(float)); offset += sizeof(float);
		memcpy(&UVScale, buffer.data() + offset, sizeof(glm::vec2)); offset += sizeof(glm::vec2);
		memcpy(&UVOffset, buffer.data() + offset, sizeof(glm::vec2)); offset += sizeof(glm::vec2);

		memcpy(&albedoTextureUUID, buffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&normalTextureUUID, buffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&metallicTextureUUID, buffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&roughnessTextureUUID, buffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&aoTextureUUID, buffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&heightTextureUUID, buffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);

		albedoTexture = AssetRegistry::GetAsset<TextureAsset>(albedoTextureUUID);
		normalTexture = AssetRegistry::GetAsset<TextureAsset>(normalTextureUUID);
		metallicTexture = AssetRegistry::GetAsset<TextureAsset>(metallicTextureUUID);
		roughnessTexture = AssetRegistry::GetAsset<TextureAsset>(roughnessTextureUUID);
		aoTexture = AssetRegistry::GetAsset<TextureAsset>(aoTextureUUID);
		heightTexture = AssetRegistry::GetAsset<TextureAsset>(heightTextureUUID);

		return true;
	}

	inline void Serialize() 
	{
		std::vector<char> buffer;
		size_t offset = 0;
		GetMetaAs<MaterialMeta>()->Serialize(buffer, offset);

		auto write = [&](const void* data, size_t size) 
		{
			if (buffer.size() < offset + size) buffer.resize(offset + size);
			memcpy(buffer.data() + offset, data, size);
			offset += size;
		};

		write(&albedoColor, sizeof(glm::vec3));
		write(&metallic, sizeof(float));
		write(&roughness, sizeof(float));
		write(&ao, sizeof(float));
		write(&normalScale, sizeof(float));
		write(&UVScale, sizeof(glm::vec2));
		write(&UVOffset, sizeof(glm::vec2));

		write(&albedoTextureUUID, sizeof(AssetUUID));
		write(&normalTextureUUID, sizeof(AssetUUID));
		write(&metallicTextureUUID, sizeof(AssetUUID));
		write(&roughnessTextureUUID, sizeof(AssetUUID));
		write(&aoTextureUUID, sizeof(AssetUUID));
		write(&heightTextureUUID, sizeof(AssetUUID));

		std::ofstream out(m_Path, std::ios::binary);
		if (!out) 
		{
			LOG_ERROR("Failed to write .rmat file: {}", m_Path);
			return;
		}

		out.write(buffer.data(), buffer.size());
		out.close();
	}

private:
	glm::vec3 albedoColor = glm::vec3(1.0f);
	float metallic = 0.0f;
	float roughness = 1.0f;
	float ao = 1.0f;
	float normalScale = 1.0f;
	glm::vec2 UVScale = glm::vec2(1.0f);
	glm::vec2 UVOffset = glm::vec2(0.0f);

	AssetUUID albedoTextureUUID;
	AssetUUID normalTextureUUID;
	AssetUUID metallicTextureUUID;
	AssetUUID roughnessTextureUUID;
	AssetUUID aoTextureUUID;
	AssetUUID heightTextureUUID;

	Ref<TextureAsset> albedoTexture;
	Ref<TextureAsset> normalTexture;
	Ref<TextureAsset> metallicTexture;
	Ref<TextureAsset> roughnessTexture;
	Ref<TextureAsset> aoTexture;
	Ref<TextureAsset> heightTexture;

	std::string m_Path;

};

struct MatComponent 
{
	Ref<MaterialAsset> Material;
	AssetUUID MaterialUUID;

	void Load()
	{
		if (MaterialUUID.IsValid()) 
		{
			Material = AssetRegistry::GetAsset<MaterialAsset>(MaterialUUID);
		}
	}
};
#endif