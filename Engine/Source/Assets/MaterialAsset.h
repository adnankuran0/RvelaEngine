#pragma once
#include "Assets/Asset.h"
#include "glm/glm.hpp"
#include "Core/Ref.h"
#include "Assets/TextureAsset.h"
#include "AssetMeta.h"
#include "Assets/AssetUUID.h"
#include <memory>

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
		: Asset(std::move(meta)), m_Path(path) 
	{
		LOG_INFO("Material asset created");
	}
	~MaterialAsset()
	{
		LOG_INFO("Material asset destroed");

	}

	bool Load()
	{
		std::ifstream file(m_Path, std::ios::binary);
		if (!file)
		{
			LOG_ERROR("Failed to open .rmat: {}", m_Path);
			return false;
		}

		AssetHeader header;
		file.read(reinterpret_cast<char*>(&header), sizeof(header));
		if (header.magic != MAGIC_MATERIAL)
		{
			LOG_ERROR("Invalid material magic!");
			return false;
		}

		std::vector<char> metaBuffer(header.metaSize);
		file.read(metaBuffer.data(), header.metaSize);

		size_t offset = 0;
		GetMetaAs<MaterialMeta>()->Deserialize(metaBuffer, offset);

		std::vector<char> dataBuffer((std::istreambuf_iterator<char>(file)), {});
		offset = 0;

		memcpy(&albedoColor, dataBuffer.data() + offset, sizeof(glm::vec3)); offset += sizeof(glm::vec3);
		memcpy(&metallic, dataBuffer.data() + offset, sizeof(float)); offset += sizeof(float);
		memcpy(&roughness, dataBuffer.data() + offset, sizeof(float)); offset += sizeof(float);
		memcpy(&ao, dataBuffer.data() + offset, sizeof(float)); offset += sizeof(float);
		memcpy(&normalScale, dataBuffer.data() + offset, sizeof(float)); offset += sizeof(float);
		memcpy(&UVScale, dataBuffer.data() + offset, sizeof(glm::vec2)); offset += sizeof(glm::vec2);
		memcpy(&UVOffset, dataBuffer.data() + offset, sizeof(glm::vec2)); offset += sizeof(glm::vec2);

		memcpy(&albedoTextureUUID, dataBuffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&normalTextureUUID, dataBuffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&metallicTextureUUID, dataBuffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&roughnessTextureUUID, dataBuffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&aoTextureUUID, dataBuffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);
		memcpy(&heightTextureUUID, dataBuffer.data() + offset, sizeof(AssetUUID)); offset += sizeof(AssetUUID);

		memcpy(&useAlbedoMap, dataBuffer.data() + offset, sizeof(bool)); offset += sizeof(bool);
		memcpy(&useNormalMap, dataBuffer.data() + offset, sizeof(bool)); offset += sizeof(bool);
		memcpy(&useMetallicMap, dataBuffer.data() + offset, sizeof(bool)); offset += sizeof(bool);
		memcpy(&useRoughnessMap, dataBuffer.data() + offset, sizeof(bool)); offset += sizeof(bool);
		memcpy(&useAOMap, dataBuffer.data() + offset, sizeof(bool)); offset += sizeof(bool);
		memcpy(&useHeightMap, dataBuffer.data() + offset, sizeof(bool)); offset += sizeof(bool);

		file.close();
		return true;
	}

	void Serialize()
	{
		std::vector<char> metaBuffer;
		size_t metaOffset = 0;
		GetMetaAs<MaterialMeta>()->Serialize(metaBuffer, metaOffset);

		std::vector<char> dataBuffer;
		size_t dataOffset = 0;
		auto write = [&](const void* data, size_t size)
			{
				if (dataBuffer.size() < dataOffset + size) dataBuffer.resize(dataOffset + size);
				memcpy(dataBuffer.data() + dataOffset, data, size);
				dataOffset += size;
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

		write(&useAlbedoMap, sizeof(bool));
		write(&useNormalMap, sizeof(bool));
		write(&useMetallicMap, sizeof(bool));
		write(&useRoughnessMap, sizeof(bool));
		write(&useAOMap, sizeof(bool));
		write(&useHeightMap, sizeof(bool));

		AssetHeader header;
		header.magic = MAGIC_MATERIAL; 
		header.version = 1;
		header.type = AssetType::Material;
		header.metaSize = static_cast<uint32_t>(metaBuffer.size());
		header.dataOffset = sizeof(AssetHeader) + header.metaSize;
		header.reserved = 0;

		std::ofstream out(m_Path, std::ios::binary);
		if (!out)
		{
			LOG_ERROR("Failed to write .rmat file: {}", m_Path);
			return;
		}

		out.write(reinterpret_cast<const char*>(&header), sizeof(header));
		out.write(metaBuffer.data(), metaBuffer.size());
		out.write(dataBuffer.data(), dataBuffer.size());
		out.close();
	}

public:
	bool useAlbedoMap = false;
	bool useNormalMap = false;
	bool useMetallicMap = false;
	bool useRoughnessMap = false;
	bool useAOMap = false;
	bool useHeightMap = false;

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

	std::string m_Path;
};


