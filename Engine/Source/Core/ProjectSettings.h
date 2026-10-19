#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "Renderer/Sampler.h"

namespace rv {

enum class StretchMode
{
	Disabled = 0,
	CanvasItems = 1,
	Viewport = 2
};

enum class StretchAspect
{
	Ignore = 0,
	Keep = 1,
	KeepWidth = 2,
	KeepHeight = 3,
	Expand = 4
};

enum class WindowMode
{
	Windowed = 0,
	Minimized = 1,
	Maximized = 2,
	Fullscreen = 3
};

enum class ScalingFilter
{
	Linear = 0,
	Nearest = 1
};

struct AutoloadEntry
{
	std::string name;
	std::string path;
	bool enabled = true;
};

struct ProjectSettings
{
	static constexpr const char* AssetDirectory = "Assets";
	static constexpr const char* CacheDirectory = "Assets/.cache";
	std::string name = "Untitled";
	std::string startScene = "Assets/Scenes/main.rscene";

	uint32_t viewportWidth = 1920;
	uint32_t viewportHeight = 1080;

	uint32_t windowWidth = 1600;
	uint32_t windowHeight = 900;

	bool overrideWindowSize = false;
	uint32_t windowOverrideWidth = 1280;
	uint32_t windowOverrideHeight = 720;

	WindowMode windowMode = WindowMode::Windowed;
	bool resizable = true;
	bool borderless = false;
	bool vsync = true;
	int maxFPS = 0;

	StretchMode stretchMode = StretchMode::Viewport;
	StretchAspect stretchAspect = StretchAspect::Keep;
	ScalingFilter scalingFilter = ScalingFilter::Nearest;

	uint32_t msaaSamples = 1;
	bool fxaaEnabled = true;
	uint32_t shadowResolution = 2048;

	TextureFilter defaultTextureFilter = TextureFilter::LinearMipmap;
	TextureWrap defaultTextureWrap = TextureWrap::Repeat;
	TextureAnisotropy defaultAnisotropy = TextureAnisotropy::Aniso8x;

	std::vector<AutoloadEntry> autoloads;
};

}
