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
	void Draw(Engine* engine ,const std::filesystem::path& rootDirectory, AssetImportPipeline& importPipeline);
	void HandleFileDrop(FileDroppedEvent& event, AssetImportPipeline& importPipeline);
};

}