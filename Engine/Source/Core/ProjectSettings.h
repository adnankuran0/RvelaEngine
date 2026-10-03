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
};

}
