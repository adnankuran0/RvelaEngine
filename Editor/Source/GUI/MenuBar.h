#pragma once
#include <functional>
#include <vector>

namespace rv {

class Engine;
class AssetImportPipeline;
class EditorPanel;
class ProjectSettingsPanel;
class ProjectSelectorPanel;

struct EditorPanels
{
    std::vector<EditorPanel*> viewPanels;
    ProjectSettingsPanel* projectSettings = nullptr;
    ProjectSelectorPanel* projectSelector = nullptr;
    std::function<void()> onResetLayout = nullptr;
};

class MenuBar
{
public:
	void Draw(Engine* engine, AssetImportPipeline& assetImporter, const EditorPanels& panels);
};

}