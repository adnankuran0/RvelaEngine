#pragma once
#include "Rendering/Skybox.h"
#include "nlohmann/json.hpp"

namespace rv {

class Environment
{
public:
	bool Lighting_IBL = true;
	float Lighting_IBLIntensity = 0.5f;
	glm::vec3 Lighting_AmbientColor = glm::vec3(1.0f, 1.0f, 1.0f);
	float Lighting_AmbientIntensity = 0.5f;

	bool SSAO = true;
	float SSAO_Radius = 1.0f;
	float SSAO_Bias = 0.025f;
	float SSAO_Intensity = 1.0f;

	bool SSR = true;

	bool Bloom = true;
	float Bloom_Intensity = 1.0f;
	float Bloom_Treshold = 1.0f;
	float Bloom_Knee = 0.5f;

	float PostProcess_Exposure = 1.0f;
	float PostProcess_VignetteIntensity = 0.25f;
	float PostProcess_VignetteSmoothness = 0.5f;
	float PostProcess_ChromaticStrength = 0.01f;

	Skybox& GetSkybox() { return m_Skybox; }

	nlohmann::json Serialize();
	void Deserialize(const nlohmann::json& j);
private:
	Skybox m_Skybox;

};

}
