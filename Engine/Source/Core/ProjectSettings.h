#pragma once
#include <string>
#include <cstdint>

namespace rv {

struct ProjectSettings
{
	static constexpr const char* AssetDirectory = "Assets";
	static constexpr const char* CacheDirectory = "Assets/.cache";
	std::string name = "Untitled";
	std::string startScene = "Assets/Scenes/main.rscene";
	uint32_t windowWidth = 1600;
	uint32_t windowHeight = 900;
	bool vsync = true;

	// Rendering Settings
	uint32_t msaaSamples = 1; // 1 (Off), 2, 4, 8
	bool fxaaEnabled = true;
	uint32_t shadowResolution = 2048; // 1024, 2048, 4096
};

}
