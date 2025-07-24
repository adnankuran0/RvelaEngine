#include "rvelapch.h"
#include "Material.h"
#include "json.hpp"
#include "Utils/TextureManager.h"
#include "Core/Log.h"
#include "Assets/AssetRegistry.h"

using json = nlohmann::json;

Material::~Material()
{
    DestroyTextures();
    LOG_INFO("Material Destroyed!");
}

std::string Material::Serialize() const
{
	json j;
	j["albedoColor"] = { albedoColor.r, albedoColor.g, albedoColor.b };
	j["metallic"] = metallic;
	j["roughness"] = roughness;
	j["ao"] = ao;

    j["UVScale"] = { UVScale.x , UVScale.y };
    j["UVOffset"] = { UVOffset.x , UVOffset.y };

	j["albedoMapPath"] = albedoMapPath.GetVirtualStr();
	j["normalMapPath"] = normalMapPath.GetVirtualStr();
	j["metallicMapPath"] = metallicMapPath.GetVirtualStr();
	j["roughnessMapPath"] = roughnessMapPath.GetVirtualStr();
	j["aoMapPath"] = aoMapPath.GetVirtualStr();
    j["heightMapPath"] = heightMapPath.GetVirtualStr();

	return j.dump(4);
}

void Material::Deserialize(const std::string& jsonStr)
{
    json j = json::parse(jsonStr);

    auto color = j["albedoColor"];
    albedoColor = glm::vec3(color[0], color[1], color[2]);
    metallic = j["metallic"];
    //roughness = j["roughness"];
    roughness = 1.0f;
    ao = j["ao"];

    auto uvScale = j["UVScale"];
    UVScale = glm::vec2(uvScale[0], uvScale[1]);

    auto uvOffset = j["UVOffset"];
    UVOffset = glm::vec2(uvOffset[0], uvOffset[1]);

    albedoMapPath = VRT_PATH(j["albedoMapPath"]);
    normalMapPath = VRT_PATH(j["normalMapPath"]);
    metallicMapPath = VRT_PATH(j["metallicMapPath"]);
    roughnessMapPath = VRT_PATH(j["roughnessMapPath"]);
    aoMapPath = VRT_PATH(j["aoMapPath"]);
    heightMapPath = VRT_PATH(j["heightMapPath"]);
}

std::string MaterialData::Serialize() const
{
    json j;
    j["albedoColor"] = { albedoColor.r, albedoColor.g, albedoColor.b };
    j["metallic"] = metallic;
    j["roughness"] = roughness;
    j["ao"] = ao;

    j["UVScale"] = { UVScale.x , UVScale.y};
    j["UVOffset"] = { UVOffset.x , UVOffset.y};

    j["albedoMapPath"] = albedoMapPath.GetVirtualStr();
    j["normalMapPath"] = normalMapPath.GetVirtualStr();
    j["metallicMapPath"] = metallicMapPath.GetVirtualStr();
    j["roughnessMapPath"] = roughnessMapPath.GetVirtualStr();
    j["aoMapPath"] = aoMapPath.GetVirtualStr();
    j["heightMapPath"] = heightMapPath.GetVirtualStr();

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

    auto uvScale = j["UVScale"];
    UVScale = glm::vec2(uvScale[0], uvScale[1]);

    auto uvOffset = j["UVOffset"];
    UVOffset = glm::vec2(uvOffset[0], uvOffset[1]);

    albedoMapPath = VRT_PATH(j["albedoMapPath"]);
    normalMapPath = VRT_PATH(j["normalMapPath"]);
    metallicMapPath = VRT_PATH(j["metallicMapPath"]);
    roughnessMapPath = VRT_PATH(j["roughnessMapPath"]);
    aoMapPath = VRT_PATH(j["aoMapPath"]);
    heightMapPath = VRT_PATH(j["heightMapPath"]);
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