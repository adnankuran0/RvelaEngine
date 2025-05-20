#include "rvelapch.h"
#include "MaterialManager.h"
#include "Core/Utils/Serializer.h"
#include <fstream>
#include <iostream>
#include "Scene/Components.h"
std::unordered_map<std::string, std::weak_ptr<Material>> MaterialManager::materialMap;

void MaterialManager::CreateMaterial(const Path path, MaterialData& materialData)
{
    if (materialMap.contains(path.GetAbsoluteStr()))
    {
        if (auto existing = materialMap[path.GetAbsoluteStr()].lock())
        {
            return;
        }
        //materialMap.erase(path);
    }


    Serializer::SaveToFile(materialData, path.GetAbsoluteStr());

    return;
}

std::shared_ptr<Material> MaterialManager::LoadOrGetMaterial(const Path path)
{

    auto it = materialMap.find(path.GetAbsoluteStr());
    if (it != materialMap.end())
    {
        if (auto existing = it->second.lock())  
        {
            return existing;
        }
        materialMap.erase(it);
    }

    std::ifstream file(path.GetAbsoluteStr());
    if (!file.is_open())
    {
        std::cerr << "Material file could not be opened: " << path.GetAbsoluteStr() << std::endl;
        return nullptr;
    }

    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto material = std::make_shared<Material>();
    material->Deserialize(jsonStr);
    material->LoadTextures();

    materialMap[path.GetAbsoluteStr()] = material;

    return material;
}

void MaterialManager::UnloadMaterial(const Path path)
{
    auto it = materialMap.find(path.GetAbsoluteStr());
    if (it != materialMap.end())
    {
        materialMap.erase(it);
    }
}

void MaterialManager::ClearMaterials()
{
    materialMap.clear();
}

void MaterialManager::ClearExpiredMaterials()
{
    for (auto it = materialMap.begin(); it != materialMap.end();)
    {
        if (it->second.expired())
        {
            it = materialMap.erase(it); 
        }
        else
        {
            ++it;
        }
    }
}

size_t MaterialManager::GetMaterialCount()
{
    return materialMap.size();
}
