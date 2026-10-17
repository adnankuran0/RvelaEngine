#pragma once
#include "Renderer/Skybox.h"
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

	bool ContactShadows = true;
	float ContactShadows_Distance = 0.3f;
	float ContactShadows_Thickness = 0.05f;
	float ContactShadows_Bias = 0.005f;
	float ContactShadows_Intensity = 1.0f;
	int ContactShadows_Steps = 16;

	bool SSR = true;
	int SSR_MaxSteps = 64;
	float SSR_MaxDistance = 10.0f;
	float SSR_Thickness = 0.15f;
	float SSR_EdgeFade = 0.10f;
	float SSR_MaxRoughness = 0.6f;

	bool SSGI = true;
	float SSGI_Intensity = 1.0f;
	float SSGI_Radius = 3.0f;
	int SSGI_Steps = 16;

	bool Bloom = true;
	float Bloom_Intensity = 1.0f;
	float Bloom_Treshold = 1.0f;
	float Bloom_Knee = 0.5f;

	bool MotionBlur = true;
	float MotionBlur_Intensity = 1.0f;
	int MotionBlur_Samples = 16;
	float MotionBlur_MaxVelocity = 0.08f;

	float PostProcess_Exposure = 1.0f;
	float PostProcess_Brightness = 0.0f;
	float PostProcess_Contrast = 1.0f;
	float PostProcess_Saturation = 1.0f;
	float PostProcess_VignetteIntensity = 0.25f;
	float PostProcess_VignetteSmoothness = 0.5f;
	float PostProcess_ChromaticStrength = 0.01f;

	bool Fog_Enabled = true;
	glm::vec3 Fog_Color = glm::vec3(0.65f, 0.75f, 0.85f);
	float Fog_Density = 0.002f;
	float Fog_Start = 5.0f;
	float Fog_End = 80.0f;
	int Fog_Mode = 1; // 0: Linear, 1: Exponential, 2: Exponential Squared

	Skybox& GetSkybox() { return m_Skybox; }

	nlohmann::json Serialize();
	void Deserialize(const nlohmann::json& j);
private:
	Skybox m_Skybox;

};

}
