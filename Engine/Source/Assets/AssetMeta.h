#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include "json.hpp"
#include "AssetUUID.h"

enum struct AssetType
{
	Texture,
	Mesh,
	Material,
	Audio,
	Shader,
	Unknown
};

struct AssetMeta {
public:
	AssetUUID uuid;
	AssetType type;
};
