#pragma once
#include <filesystem>
#include "Renderer/Texture.h"
#include <Asset/AssetImportPipeline.h>
#include "Event/WindowEvents.h"

namespace rv {

class Engine;

class AssetBrowserPanel
{
public:
	AssetBrowserPanel();
	void Draw(Engine* engine ,const std::filesystem::path& rootDirectory, AssetImportPipeline& importPipeline);
	void HandleFileDrop(FileDroppedEvent& event, AssetImportPipeline& importPipeline);
private:
	Texture folderIcon;
	Texture materialIcon;
	Texture sceneIcon;
	Texture scriptIcon;
	Texture textureIcon;
	Texture meshIcon;
	Texture prefabIcon;
};

}