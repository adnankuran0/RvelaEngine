#pragma once
#include "EditorPanel.h"
#include <filesystem>
#include <string>
#include "AssetImporters/TextureImporter.h"
#include "AssetImporters/ModelImporter.h"

namespace rv {

class AssetImportPipeline;

class ImportSettingsPanel : public EditorPanel
{
public:
    ImportSettingsPanel() : EditorPanel("Import Settings", true) {}
    ~ImportSettingsPanel() override = default;

    void Draw(AssetImportPipeline& importPipeline);

    static void SetInspectedAsset(const std::filesystem::path& assetPath);
    static const std::filesystem::path& GetInspectedAsset();

    static bool DrawAssetSettingsUI(
        const std::filesystem::path& assetPath,
        AssetImportPipeline& importPipeline,
        bool isEmbedded = false);
};

}