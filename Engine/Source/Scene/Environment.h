#pragma once

namespace rv {

class Environment
{
public:
	bool SSAO = true;
	float SSAO_Radius = 1.0f;
	float SSAO_Bias = 0.025f;
	float SSAO_Intensity = 1.0f;

	bool SSR = true;

	bool Bloom = true;
	float Bloom_Treshold = 1.0;
	float Bloom_Knee = 0.5f;

	float PostProcess_Exposure = 1.0f;
	float PostProcess_VignetteIntensity = 0.25f;
	float PostProcess_VignetteSmoothness = 0.5f;
	float PostProcess_ChromaticStrength = 0.01f;
};

}
