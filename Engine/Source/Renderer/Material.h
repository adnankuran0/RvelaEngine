#pragma once
#include "glm/glm.hpp"
#include "Shader.h"
#include "Renderer/Texture.h"
#include <memory>
#include "Core/ISerializable.h"

class Material : public ISerializable
{
public:
	std::shared_ptr<Shader> shader;

	~Material();

	glm::vec3 albedoColor = glm::vec3(1.0f);
	float metallic = 0.0f;
	float roughness = 1.0f;
	float ao = 1.0f;

	std::string albedoMapPath = "";
	std::string normalMapPath = "";
	std::string metallicMapPath = "";
	std::string roughnessMapPath = "";
	std::string aoMapPath = "";
	std::string heightMapPath = "";

	void LoadTextures();
	std::string Serialize() const override;
	void Deserialize(const std::string& jsonStr) override;
private:
	void DestroyTextures();


};