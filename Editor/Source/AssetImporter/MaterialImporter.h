#pragma once
#include "AssetImporter.h"
#include "Core/Ref.h"
#include "Assets/MaterialAsset.h"
#include <assimp/scene.h>

class MaterialImporter 
{
public:
	static Ref<MaterialAsset> CreateMaterialAsset(const std::string& path);
};