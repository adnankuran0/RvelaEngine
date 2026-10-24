#pragma once

namespace rv {

class Engine;
class AssetImportPipeline;
class ProjectSettingsPanel;
class ProjectSelectorPanel;
class ImportSettingsPanel;
class MaterialEditorPanel;

class MenuBar
{
public:
	void Draw(Engine* engine, AssetImportPipeline& assetImporter, 
		ProjectSettingsPanel* projectSettingsPanel = nullptr,
		ProjectSelectorPanel* projectSelectorPanel = nullptr,
		ImportSettingsPanel* importSettingsPanel = nullptr,
		MaterialEditorPanel* materialEditorPanel = nullptr);
};

}
