#pragma once
#include "glm/glm.hpp"
#include "Shader.h"
#include "Renderer/Texture.h"
#include <memory>
class Material
{
public:
	std::shared_ptr<Shader> shader;

	glm::vec3 albedoColor = glm::vec3(1.0f);
	float metallic = 0.0f;
	float roughness = 1.0f;
	float ao = 1.0f;

	std::shared_ptr<Texture> albedoMap = nullptr;
	std::shared_ptr<Texture> normalMap = nullptr;
	std::shared_ptr<Texture> metallicMap = nullptr;
	std::shared_ptr<Texture> roughnessMap = nullptr;
	std::shared_ptr<Texture> aoMap = nullptr;



};