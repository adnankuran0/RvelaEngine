#include "rvelapch.h"
#include "Material.h"
#include "../nlohmann/json.hpp"
#include "Core/Utils/TextureManager.h"

using json = nlohmann::json;

Material::~Material()
{
    DestroyTextures();

}

std::string Material::Serialize() const
{
	json j;
	j["albedoColor"] = { albedoColor.r, albedoColor.g, albedoColor.b };
	j["metallic"] = metallic;
	j["roughness"] = roughness;
	j["ao"] = ao;



	j["albedoMapPath"] = albedoMapPath;
	j["normalMapPath"] = normalMapPath;
	j["metallicMapPath"] = metallicMapPath;
	j["roughnessMapPath"] = roughnessMapPath;
	j["aoMapPath"] = aoMapPath;
	j["heightMapPath"] = heightMapPath;

	return j.dump(4);
}

void Material::Deserialize(const std::string& jsonStr)
{
    json j = json::parse(jsonStr);

    auto color = j["albedoColor"];
    albedoColor = glm::vec3(color[0], color[1], color[2]);
    metallic = j["metallic"];
    roughness = j["roughness"];
    ao = j["ao"];

   

    albedoMapPath = j["albedoMapPath"];
    normalMapPath = j["normalMapPath"];
    metallicMapPath = j["metallicMapPath"];
    roughnessMapPath = j["roughnessMapPath"];
    aoMapPath = j["aoMapPath"];
    heightMapPath = j["heightMapPath"];
}

std::string MaterialData::Serialize() const
{
    json j;
    j["albedoColor"] = { albedoColor.r, albedoColor.g, albedoColor.b };
    j["metallic"] = metallic;
    j["roughness"] = roughness;
    j["ao"] = ao;



    j["albedoMapPath"] = albedoMapPath;
    j["normalMapPath"] = normalMapPath;
    j["metallicMapPath"] = metallicMapPath;
    j["roughnessMapPath"] = roughnessMapPath;
    j["aoMapPath"] = aoMapPath;
    j["heightMapPath"] = heightMapPath;

    return j.dump(4);
}

void MaterialData::Deserialize(const std::string& jsonStr)
{
    json j = json::parse(jsonStr);

    auto color = j["albedoColor"];
    albedoColor = glm::vec3(color[0], color[1], color[2]);
    metallic = j["metallic"];
    roughness = j["roughness"];
    ao = j["ao"];



    albedoMapPath = j["albedoMapPath"];
    normalMapPath = j["normalMapPath"];
    metallicMapPath = j["metallicMapPath"];
    roughnessMapPath = j["roughnessMapPath"];
    aoMapPath = j["aoMapPath"];
    heightMapPath = j["heightMapPath"];
}

void Material::LoadTextures()
{
    TextureManager::LoadOrGetTexture(albedoMapPath);
    TextureManager::LoadOrGetTexture(normalMapPath);
    TextureManager::LoadOrGetTexture(metallicMapPath);
    TextureManager::LoadOrGetTexture(roughnessMapPath);
    TextureManager::LoadOrGetTexture(aoMapPath);
    TextureManager::LoadOrGetTexture(heightMapPath);
}

void Material::DestroyTextures()
{
    TextureManager::UnloadTexture(albedoMapPath);
    TextureManager::UnloadTexture(normalMapPath);
    TextureManager::UnloadTexture(metallicMapPath);
    TextureManager::UnloadTexture(roughnessMapPath);
    TextureManager::UnloadTexture(aoMapPath);
    TextureManager::UnloadTexture(heightMapPath);
}