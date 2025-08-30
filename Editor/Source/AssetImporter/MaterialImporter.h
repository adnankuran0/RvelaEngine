#pragma once
#include "AssetImporter.h"
#include "Core/Ref.h"
#include "Assets/MaterialAsset.h"
#include <assimp/scene.h>
#include "Core/Singleton.h"

class MaterialImporter : public Singleton<MaterialImporter>
{
public:
	Ref<MaterialAsset> CreateMaterialAsset(const std::string& path);
};