#pragma once
#include <filesystem>
#include "Core/Engine.h"

class AssetBrowserPanel
{
public:
	void Draw(Engine* engine ,const std::filesystem::path& rootDirectory);

};
