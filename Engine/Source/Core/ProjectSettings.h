#pragma once
#include <string>
#include <vector>
#include <array>
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

	std::array<std::string, 32> physicsLayerNames = {
		"Default", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", "",
		"", "", "", "", "", "", "", ""
	};

	const std::string& GetPhysicsLayerName(uint32_t layer1Based) const
	{
		static const std::string s_Empty = "";
		if (layer1Based >= 1 && layer1Based <= 32)
			return physicsLayerNames[layer1Based - 1];
		return s_Empty;
	}

	const std::string& GetPhysicsLayerNameByBit(uint32_t bitIndex) const
	{
		static const std::string s_Empty = "";
		if (bitIndex < 32)
			return physicsLayerNames[bitIndex];
		return s_Empty;
	}

	void SetPhysicsLayerName(uint32_t layer1Based, const std::string& name)
	{
		if (layer1Based >= 1 && layer1Based <= 32)
			physicsLayerNames[layer1Based - 1] = name;
	}

	uint32_t GetPhysicsLayerMask(const std::string& name) const
	{
		if (name.empty()) return 0;
		for (size_t i = 0; i < physicsLayerNames.size(); ++i)
		{
			if (physicsLayerNames[i] == name)
				return 1u << i;
		}
		return 0;
	}
};

}
