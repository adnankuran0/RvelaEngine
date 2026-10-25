#pragma once
#include "EditorPanel.h"
#include <filesystem>
#include <vector>
#include "Renderer/Texture.h"
#include <Asset/AssetImportPipeline.h>
#include "Event/WindowEvents.h"

namespace rv {

class Engine;

class AssetBrowserPanel : public EditorPanel
{
public:
	AssetBrowserPanel() : EditorPanel("Asset Browser", true) {}

	void Draw(Engine* engine, const std::filesystem::path& rootDirectory, AssetImportPipeline& importPipeline);
	void HandleFileDrop(FileDroppedEvent& event, AssetImportPipeline& importPipeline);

	static void RequestRename(const std::filesystem::path& path);
	static void RequestRefresh();
	static void Reset(const std::filesystem::path& rootDirectory = {});
	static const std::vector<std::filesystem::path>& GetSelectedPaths();
	static std::filesystem::path GetSelectedPrimaryPath();
	static void ClearSelection();
	static void OpenImportSettings(const std::filesystem::path& path);
};

}