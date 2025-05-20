#include "rvelapch.h"
#include "Material.h"
#include "../nlohmann/json.hpp"
#include "Core/Utils/TextureManager.h"
#include "RvelaLog.h"

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

    j["UVScale"] = { UVScale.x , UVScale.y, UVScale.z };
    j["UVOffset"] = { UVOffset.x , UVOffset.y, UVOffset.z };

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
    roughness = j["roughness"];
    ao = j["ao"];

    auto uvScale = j["UVScale"];
    UVScale = glm::vec3(uvScale[0], uvScale[1], uvScale[2]);

    auto uvOffset = j["UVOffset"];
    UVOffset = glm::vec3(uvOffset[0], uvOffset[1], uvOffset[2]);

    albedoMapPath = TO_ABSOLUTE_PATH(j["albedoMapPath"]);
    normalMapPath = TO_ABSOLUTE_PATH(j["normalMapPath"]);
    metallicMapPath = TO_ABSOLUTE_PATH(j["metallicMapPath"]);
    roughnessMapPath = TO_ABSOLUTE_PATH(j["roughnessMapPath"]);
    aoMapPath = TO_ABSOLUTE_PATH(j["aoMapPath"]);
    heightMapPath = TO_ABSOLUTE_PATH(j["heightMapPath"]);
}

std::string MaterialData::Serialize() const
{
    json j;
    j["albedoColor"] = { albedoColor.r, albedoColor.g, albedoColor.b };
    j["metallic"] = metallic;
    j["roughness"] = roughness;
    j["ao"] = ao;

    j["UVScale"] = { UVScale.x , UVScale.y, UVScale.z };
    j["UVOffset"] = { UVOffset.x , UVOffset.y, UVOffset.z };

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
    UVScale = glm::vec3(uvScale[0], uvScale[1], uvScale[2]);

    auto uvOffset = j["UVOffset"];
    UVOffset = glm::vec3(uvOffset[0], uvOffset[1], uvOffset[2]);

    albedoMapPath = TO_ABSOLUTE_PATH(j["albedoMapPath"]);
    normalMapPath = TO_ABSOLUTE_PATH(j["normalMapPath"]);
    metallicMapPath = TO_ABSOLUTE_PATH(j["metallicMapPath"]);
    roughnessMapPath = TO_ABSOLUTE_PATH(j["roughnessMapPath"]);
    aoMapPath = TO_ABSOLUTE_PATH(j["aoMapPath"]);
    heightMapPath = TO_ABSOLUTE_PATH(j["heightMapPath"]);
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