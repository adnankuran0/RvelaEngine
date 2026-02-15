#pragma once
#include <filesystem>

namespace rv {

class Engine;

class AssetBrowserPanel
{
public:
	void Draw(Engine* engine ,const std::filesystem::path& rootDirectory);

};

}