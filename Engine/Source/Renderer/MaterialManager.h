#pragma once
#include "Scene/Components.h"
#include "Material.h"
#include <fstream>

class MaterialManager
{
public:
	static std::shared_ptr<Material> CreateMaterial(const std::string& path);
	static std::shared_ptr<Material> LoadOrGetMaterial(const std::string& path);
	static void UnloadMaterial(const std::string& path);
	static void ClearMaterials();
	static size_t GetMaterialCount();
private:
	static std::unordered_map<std::string, std::shared_ptr<Material>> materialMap;
};