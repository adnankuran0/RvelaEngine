#pragma once
#include <filesystem>
#include "Core/Engine.h"

class AssetBrowserPanel
{
public:
	static void Draw(Engine* engine ,const std::filesystem::path& rootDirectory);

};
