#pragma once
#include <string>
#include <filesystem>
#include "AssetUUID.h"

#define MAGIC_TEXTURE 0x52544558

enum struct AssetType : uint8_t
{
	Unknown = 0,
	Texture,
	Mesh,
	Material,
	Audio,
	Shader,
	Scene,
	Prefab,
};

struct AssetHeader
{
	uint32_t magic; 
	uint16_t version;
	AssetType type;
	uint8_t reserved;		// Alignment

	uint32_t metaSize;     
	uint32_t dataOffset;   // Offset to binary data 
	// 16 bytes total (aligned)
};

class AssetMeta
{
public:
	virtual ~AssetMeta() = default;
	virtual std::unique_ptr<AssetMeta> Clone() const = 0;

	AssetUUID uuid;
	std::filesystem::path sourcePath;
	AssetType type = AssetType::Unknown;
};