#pragma once
#include "glm/glm.hpp"
#include "Shader.h"
#include "Renderer/Texture.h"
#include <memory>
#include "Utils/ISerializable.h"

class Material : public ISerializable
{
public:

	~Material();
	Material()
	{
	}
	Material(const Material&) { std::cout << "Material copied! (" << this << ")\n"; }

	glm::vec3 albedoColor = glm::vec3(1.0f);
	float metallic = 0.0f;
	float roughness = 1.0f;
	float ao = 1.0f;
	float normalScale = 1.0f;

	glm::vec2 UVScale = glm::vec2(1.0f);
	glm::vec2 UVOffset = glm::vec2(0.0f);

	Path albedoMapPath;
	Path normalMapPath;
	Path metallicMapPath;
	Path roughnessMapPath;
	Path aoMapPath;
	Path heightMapPath;

	void LoadTextures();
	std::string Serialize() const override;
	void Deserialize(const std::string& jsonStr) override;
private:
	void DestroyTextures();
};

class MaterialData : public ISerializable
{
public:
	glm::vec3 albedoColor = glm::vec3(1.0f);
	float metallic = 0.0f;
	float roughness = 0.5f;
	float ao = 1.0f;
	float normalScale = 1.0f;

	glm::vec2 UVScale = glm::vec2(1.0f);
	glm::vec2 UVOffset = glm::vec2(0.0f);

	Path albedoMapPath;
	Path normalMapPath;
	Path metallicMapPath;
	Path roughnessMapPath;
	Path aoMapPath;
	Path heightMapPath;

	std::string Serialize() const override;
	void Deserialize(const std::string& jsonStr) override;
};

