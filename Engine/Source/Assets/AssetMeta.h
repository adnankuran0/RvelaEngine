#pragma once
#include <string>
#include <filesystem>
#include "AssetUUID.h"

constexpr uint32_t MAGIC_TEXTURE  = 0x52544558; // RTEX ASCII
constexpr uint32_t MAGIC_MATERIAL = 0x524D4154; // RMAT ASCII
constexpr uint32_t MAGIC_MESH     = 0x524D5348; // RMSH ASCII
constexpr uint32_t MAGIC_PREFAB   = 0x52504642; // RPFB ASCII

enum struct AssetType : uint8_t
{
	Unknown,
	Texture,
	Mesh,
	Material,
	Prefab,
	Audio,
	Shader,
	Scene,
};

struct AssetHeader
{
	uint32_t magic; 
	uint16_t version;
	AssetType type;
	uint8_t reserved;// Alignment
	uint32_t metaSize;     
	uint32_t dataOffset;   
};

class AssetMeta
{
public:
	virtual ~AssetMeta() = default;
	virtual std::unique_ptr<AssetMeta> Clone() const = 0;

	virtual void Serialize(std::vector<char>& outBuffer, size_t& offset) const = 0;
	virtual void Deserialize(const std::vector<char>& inBuffer, size_t& offset) = 0;

protected:
	void SerializeBase(std::vector<char>& outBuffer, size_t& offset) const
	{
		std::string pathStr = sourcePath.string();
		uint32_t pathSize = static_cast<uint32_t>(pathStr.size());

		size_t totalSize = sizeof(uuid) + sizeof(pathSize) + pathSize + sizeof(type);
		if (outBuffer.size() < offset + totalSize)
			outBuffer.resize(offset + totalSize);

		memcpy(outBuffer.data() + offset, &uuid, sizeof(uuid));
		offset += sizeof(uuid);

		memcpy(outBuffer.data() + offset, &pathSize, sizeof(pathSize));
		offset += sizeof(pathSize);

		memcpy(outBuffer.data() + offset, pathStr.data(), pathSize);
		offset += pathSize;

		memcpy(outBuffer.data() + offset, &type, sizeof(type));
		offset += sizeof(type);
	}

	void DeserializeBase(const std::vector<char>& inBuffer, size_t& offset)
	{
		memcpy(&uuid, inBuffer.data() + offset, sizeof(uuid));
		offset += sizeof(uuid);

		uint32_t pathSize;
		memcpy(&pathSize, inBuffer.data() + offset, sizeof(pathSize));
		offset += sizeof(pathSize);

		std::string pathStr(pathSize, '\0');
		memcpy(pathStr.data(), inBuffer.data() + offset, pathSize);
		offset += pathSize;
		sourcePath = pathStr;

		memcpy(&type, inBuffer.data() + offset, sizeof(type));
		offset += sizeof(type);
	}

public:
	AssetUUID uuid;
	std::filesystem::path sourcePath;
	AssetType type = AssetType::Unknown;
};