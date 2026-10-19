#pragma once
#include <filesystem>
#include <string>
#include "AssetImporters/TextureImporter.h"
#include "AssetImporters/ModelImporter.h"

namespace rv {

class AssetImportPipeline;

class ImportSettingsPanel
{
public:
    ImportSettingsPanel() = default;
    ~ImportSettingsPanel() = default;

    void Draw(AssetImportPipeline& importPipeline);

    void Open() { m_IsOpen = true; }
    void Close() { m_IsOpen = false; }
    bool IsOpen() const { return m_IsOpen; }
    void Toggle() { m_IsOpen = !m_IsOpen; }

    static void SetInspectedAsset(const std::filesystem::path& assetPath);
    static const std::filesystem::path& GetInspectedAsset();

    static bool DrawAssetSettingsUI(
        const std::filesystem::path& assetPath,
        AssetImportPipeline& importPipeline,
        bool isEmbedded = false);

private:
    bool m_IsOpen = true;
};

}
