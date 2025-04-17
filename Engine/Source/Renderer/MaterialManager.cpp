#include "rvelapch.h"
#include "MaterialManager.h"
#include "Core/Serializer.h"
std::unordered_map<std::string, std::shared_ptr<Material>> MaterialManager::materialMap;


std::shared_ptr<Material> MaterialManager::CreateMaterial(const std::string& path)
{
    if (materialMap.contains(path))
        return materialMap[path];

    auto mat = std::make_shared<Material>();
    materialMap[path] = mat;

    Serializer::SaveToFile(*mat, path);

    return mat;
}

std::shared_ptr<Material> MaterialManager::LoadOrGetMaterial(const std::string& path)
{
	//checking if texture already exists
    auto it = materialMap.find(path);
    if (it != materialMap.end()) {
        return it->second;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Material file could not be opened: " << path << std::endl;
        return nullptr;
    }

    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto material = std::make_shared<Material>();
    material->Deserialize(jsonStr);
    material->LoadTextures();

    materialMap[path] = material;

    

    return material;
}

void MaterialManager::UnloadMaterial(const std::string& path)
{
    auto it = materialMap.find(path);
    if (it != materialMap.end()) {
        materialMap.erase(it);
    }
}

void MaterialManager::ClearMaterials() {
    materialMap.clear();
}

size_t MaterialManager::GetMaterialCount() 
{
    return materialMap.size();
}