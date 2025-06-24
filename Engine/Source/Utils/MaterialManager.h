#pragma once
#include "Renderer/Material.h"
#include <unordered_map>

class MaterialManager
{
public:
    static void CreateMaterial(const Path path,MaterialData& materialData);
    static std::shared_ptr<Material> LoadOrGetMaterial(const Path path);
    static void UnloadMaterial(const Path path);
    static void ClearMaterials();
    static size_t GetMaterialCount();

    static void ClearExpiredMaterials();

private:
    static std::unordered_map<std::string, std::weak_ptr<Material>> materialMap;
};
