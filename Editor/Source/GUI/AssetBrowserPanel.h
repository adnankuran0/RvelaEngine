#pragma once
#include <filesystem>

class Engine;

class AssetBrowserPanel
{
public:
	void Draw(Engine* engine ,const std::filesystem::path& rootDirectory);

};
