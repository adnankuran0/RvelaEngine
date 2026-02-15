#pragma once
#include "Core/Ref.h"
#include "Core/Singleton.h"
#include <string>

namespace rv {

class MaterialAsset;

class MaterialImporter : public Singleton<MaterialImporter>
{
public:
	Ref<MaterialAsset> CreateMaterialAsset(const std::string& path);
};

}