#pragma once

#include "Renderer/Material.h"
#include <fstream>
#include <unordered_map>
#include <memory>

class MaterialManager
{
public:
    static void CreateMaterial(const std::string& path,MaterialData& materialData);
    static std::shared_ptr<Material> LoadOrGetMaterial(const std::string& path);
    static void UnloadMaterial(const std::string& path);
    static void ClearMaterials();
    static size_t GetMaterialCount();

    static void ClearExpiredMaterials();

private:
    static std::unordered_map<std::string, std::weak_ptr<Material>> materialMap;
};
