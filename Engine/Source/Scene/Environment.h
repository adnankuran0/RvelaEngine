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
	bool PostProcess_AutoExposure = false;
	float PostProcess_AutoExposureMin = 0.2f;
	float PostProcess_AutoExposureMax = 5.0f;
	float PostProcess_AutoExposureSpeedUp = 3.0f;
	float PostProcess_AutoExposureSpeedDown = 1.0f;
	float PostProcess_AutoExposureTarget = 0.18f;
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

	void CopySettingsFrom(const Environment& other)
	{
		Lighting_IBL = other.Lighting_IBL;
		Lighting_IBLIntensity = other.Lighting_IBLIntensity;
		Lighting_AmbientColor = other.Lighting_AmbientColor;
		Lighting_AmbientIntensity = other.Lighting_AmbientIntensity;

		SSAO = other.SSAO;
		SSAO_Radius = other.SSAO_Radius;
		SSAO_Bias = other.SSAO_Bias;
		SSAO_Intensity = other.SSAO_Intensity;

		ContactShadows = other.ContactShadows;
		ContactShadows_Distance = other.ContactShadows_Distance;
		ContactShadows_Thickness = other.ContactShadows_Thickness;
		ContactShadows_Bias = other.ContactShadows_Bias;
		ContactShadows_Intensity = other.ContactShadows_Intensity;
		ContactShadows_Steps = other.ContactShadows_Steps;

		SSR = other.SSR;
		SSR_MaxSteps = other.SSR_MaxSteps;
		SSR_MaxDistance = other.SSR_MaxDistance;
		SSR_Thickness = other.SSR_Thickness;
		SSR_EdgeFade = other.SSR_EdgeFade;
		SSR_MaxRoughness = other.SSR_MaxRoughness;

		SSGI = other.SSGI;
		SSGI_Intensity = other.SSGI_Intensity;
		SSGI_Radius = other.SSGI_Radius;
		SSGI_Steps = other.SSGI_Steps;

		Bloom = other.Bloom;
		Bloom_Intensity = other.Bloom_Intensity;
		Bloom_Treshold = other.Bloom_Treshold;
		Bloom_Knee = other.Bloom_Knee;

		MotionBlur = other.MotionBlur;
		MotionBlur_Intensity = other.MotionBlur_Intensity;
		MotionBlur_Samples = other.MotionBlur_Samples;
		MotionBlur_MaxVelocity = other.MotionBlur_MaxVelocity;

		PostProcess_Exposure = other.PostProcess_Exposure;
		PostProcess_AutoExposure = other.PostProcess_AutoExposure;
		PostProcess_AutoExposureMin = other.PostProcess_AutoExposureMin;
		PostProcess_AutoExposureMax = other.PostProcess_AutoExposureMax;
		PostProcess_AutoExposureSpeedUp = other.PostProcess_AutoExposureSpeedUp;
		PostProcess_AutoExposureSpeedDown = other.PostProcess_AutoExposureSpeedDown;
		PostProcess_AutoExposureTarget = other.PostProcess_AutoExposureTarget;
		PostProcess_Brightness = other.PostProcess_Brightness;
		PostProcess_Contrast = other.PostProcess_Contrast;
		PostProcess_Saturation = other.PostProcess_Saturation;
		PostProcess_VignetteIntensity = other.PostProcess_VignetteIntensity;
		PostProcess_VignetteSmoothness = other.PostProcess_VignetteSmoothness;
		PostProcess_ChromaticStrength = other.PostProcess_ChromaticStrength;

		Fog_Enabled = other.Fog_Enabled;
		Fog_Color = other.Fog_Color;
		Fog_Density = other.Fog_Density;
		Fog_Start = other.Fog_Start;
		Fog_End = other.Fog_End;
		Fog_Mode = other.Fog_Mode;
	}

	Environment& operator=(const Environment& other)
	{
		if (this != &other)
		{
			CopySettingsFrom(other);
		}
		return *this;
	}

	Skybox& GetSkybox() { return m_Skybox; }

	nlohmann::json Serialize();
	void Deserialize(const nlohmann::json& j);
private:
	Skybox m_Skybox;

};

}
