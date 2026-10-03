#pragma once
#include <string>
#include <cstdint>

namespace rv {

struct ProjectSettings
{
	std::string name = "Untitled";
	std::string startScene = "";
	std::string assetDirectory = "Assets";
	std::string cacheDirectory = "Assets/.cache";
	uint32_t windowWidth = 1600;
	uint32_t windowHeight = 900;
	bool vsync = true;
};

}
